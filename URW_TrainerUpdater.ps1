param([string]$CurrentVersion='0.21',[string]$TrainerPath='', [int]$TrainerPid=0, [switch]$LibraryOnly)
$ErrorActionPreference='Stop'
$script:TrainerRepository='Librianets/UnRealWorld_QuestTrainer'
$script:CompactFiles=@('URW_QuestTrainer.exe','URW_TrainerUpdater.ps1','README.txt','CODE_LICENSE.txt','build.cmd','src/QuestTrainer.cpp','src/QuestPlan.h','src/NativeState.h','src/TrainerFlow.h','src/HookState.h')
$script:LegacyFullSourceFiles=@($script:CompactFiles)+@('src/CharacterState.h','src/CharacterWindows.h','src/ItemCatalogue.h','src/TimeAdvance.h')
$script:FullSourceFiles=@($script:LegacyFullSourceFiles)+@('src/AnimalPolicy.h','src/AnimalSpawn.h')
function Get-TrainerExpectedFiles($Manifest){
 # The10-file compact source layout lets the original015/016 updater install017.
 # Accept the historical14-file layout and the new16-file split source layout.
 $count=@($Manifest.files.psobject.Properties.Name).Count
 if($count -eq $script:CompactFiles.Count){return $script:CompactFiles}
 if($count -eq $script:LegacyFullSourceFiles.Count){return $script:LegacyFullSourceFiles}
 if($count -eq $script:FullSourceFiles.Count){return $script:FullSourceFiles}
 throw 'Incomplete update file list.'
}

function Read-TrainerVersion([string]$Value){
 if($Value -notmatch '^\d+\.\d{2}$'){throw 'Invalid trainer version.'}
 return [version]$Value
}
function Test-TrainerManifest($Manifest){
 if($Manifest.schema -ne 1 -or $Manifest.repository -cne $script:TrainerRepository){throw 'Wrong update manifest.'}
 $null=Read-TrainerVersion ([string]$Manifest.version)
 if($Manifest.archive -cne ('URW_QuestTrainer_'+$Manifest.version+'.zip') -or $Manifest.sha256 -notmatch '^[a-f0-9]{64}$'){throw 'Invalid archive identity.'}
 $names=@($Manifest.files.psobject.Properties.Name)
 $expected=@(Get-TrainerExpectedFiles $Manifest)
 foreach($name in $names){
  if($expected -cnotcontains $name -or $Manifest.files.$name -notmatch '^[a-f0-9]{64}$'){throw 'Unsupported update file.'}
 }
}
function Test-TrainerArchive([string]$Archive,$Manifest){
 Test-TrainerManifest $Manifest
 $expected=@(Get-TrainerExpectedFiles $Manifest)
 if((Get-FileHash -Algorithm SHA256 -LiteralPath $Archive).Hash.ToLowerInvariant() -cne $Manifest.sha256){throw 'Archive SHA256 mismatch.'}
 Add-Type -AssemblyName System.IO.Compression.FileSystem
 $z=[IO.Compression.ZipFile]::OpenRead($Archive)
 try{
  $seen=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
  foreach($entry in $z.Entries){
   $name=$entry.FullName
   if($expected -cnotcontains $name -or -not $seen.Add($name)){throw 'Unexpected or duplicate ZIP entry.'}
   if($entry.Length -gt 16777216){throw 'Update file exceeds size limit.'}
   $stream=$entry.Open();$hash=[Security.Cryptography.SHA256]::Create()
   try{$actual=([BitConverter]::ToString($hash.ComputeHash($stream))).Replace('-','').ToLowerInvariant()}finally{$stream.Dispose();$hash.Dispose()}
   if($actual -cne $Manifest.files.$name){throw 'Update file SHA256 mismatch.'}
  }
  if($seen.Count -ne $expected.Count){throw 'Missing update file.'}
 }finally{$z.Dispose()}
}
function Install-TrainerFiles([string]$Stage,[string]$Destination,$Manifest){
 Test-TrainerManifest $Manifest
 $expected=@(Get-TrainerExpectedFiles $Manifest)
 $backup=Join-Path $Destination 'URW_Trainer_backup'
 if(Test-Path -LiteralPath $backup){Remove-Item -LiteralPath $backup -Recurse -Force}
 $null=New-Item -ItemType Directory -Path $backup
 $changed=New-Object 'System.Collections.Generic.List[string]'
 try{
  foreach($name in $expected){
   $target=Join-Path $Destination $name;$old=Join-Path $backup $name
   if(Test-Path -LiteralPath $target){$null=New-Item -ItemType Directory -Force -Path (Split-Path $old);Copy-Item -LiteralPath $target -Destination $old}
   $changed.Add($name);$null=New-Item -ItemType Directory -Force -Path (Split-Path $target)
   Copy-Item -LiteralPath (Join-Path $Stage $name) -Destination $target -Force
   if((Get-FileHash -Algorithm SHA256 -LiteralPath $target).Hash.ToLowerInvariant() -cne $Manifest.files.$name){throw 'Installed file hash mismatch.'}
  }
 }catch{
  foreach($name in $changed){$old=Join-Path $backup $name;$target=Join-Path $Destination $name
   if(Test-Path -LiteralPath $old){Copy-Item -LiteralPath $old -Destination $target -Force}else{Remove-Item -LiteralPath $target -Force -ErrorAction SilentlyContinue}
  }
  throw
 }
}
if($LibraryOnly){return}
Add-Type -AssemblyName System.Windows.Forms
function Notice([string]$Text){$null=[Windows.Forms.MessageBox]::Show($Text,'URW Quest Trainer',[Windows.Forms.MessageBoxButtons]::OK,[Windows.Forms.MessageBoxIcon]::Information)}
$stage=$null;$mutex=$null;$locked=$false
try{
 [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
 $root=[IO.Path]::GetFullPath((Split-Path -Parent $TrainerPath))
 if([IO.Path]::GetFileName($TrainerPath) -cne 'URW_QuestTrainer.exe' -or -not(Test-Path -LiteralPath $TrainerPath)){throw 'Trainer executable not found.'}
 $sha=[Security.Cryptography.SHA256]::Create();try{$id=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($root.ToLowerInvariant())))).Replace('-','')}finally{$sha.Dispose()}
 $mutex=New-Object Threading.Mutex($false,('Local\URW_Trainer_Update_'+$id));$locked=$mutex.WaitOne(0)
 if(-not $locked){throw 'An update is already running.'}
 $headers=@{'User-Agent'='URW-QuestTrainer-Updater';'Accept'='application/vnd.github+json'}
 # Release-asset URLs work without the rate-limited REST API or an account.
 $response=Invoke-WebRequest -UseBasicParsing -Uri ('https://github.com/'+$script:TrainerRepository+'/releases/latest/download/trainer-update.json') -Headers $headers -TimeoutSec 30
 $json=if($response.Content -is [byte[]]){[Text.Encoding]::UTF8.GetString($response.Content)}else{[string]$response.Content}
 $manifest=$json|ConvertFrom-Json
 Test-TrainerManifest $manifest
 if((Read-TrainerVersion $manifest.version) -le (Read-TrainerVersion $CurrentVersion)){Notice ('Установлена последняя версия тренера: '+$CurrentVersion);return}
 $answer=[Windows.Forms.MessageBox]::Show(('Доступен тренер '+$manifest.version+'. Скачать и установить? Тренер будет закрыт и запущен снова. Игра продолжит работать.'),'URW Quest Trainer',[Windows.Forms.MessageBoxButtons]::YesNo,[Windows.Forms.MessageBoxIcon]::Question)
 if($answer -ne [Windows.Forms.DialogResult]::Yes){return}
 $archiveUrl='https://github.com/'+$script:TrainerRepository+'/releases/download/trainer-'+$manifest.version+'/'+$manifest.archive
 $stage=Join-Path ([IO.Path]::GetTempPath()) ('URWTrainer_'+[guid]::NewGuid().ToString('N'));$null=New-Item -ItemType Directory -Path $stage
 $archive=Join-Path $stage $manifest.archive
 Invoke-WebRequest -UseBasicParsing -Uri $archiveUrl -OutFile $archive -Headers $headers -TimeoutSec 120
 Test-TrainerArchive $archive $manifest
 $expanded=Join-Path $stage 'files';[IO.Compression.ZipFile]::ExtractToDirectory($archive,$expanded)
 $parent=Get-Process -Id $TrainerPid -ErrorAction SilentlyContinue
 if($parent){
  if([IO.Path]::GetFullPath($parent.Path) -ine [IO.Path]::GetFullPath($TrainerPath)){throw 'Trainer process/path mismatch.'}
  $null=$parent.CloseMainWindow();if(-not $parent.WaitForExit(10000)){throw 'Close the trainer before updating.'}
 }
 Install-TrainerFiles $expanded $root $manifest
 Start-Process -FilePath $TrainerPath -WorkingDirectory $root
 Notice ('Тренер обновлён до '+$manifest.version+'.')
}catch{Notice ('Обновление не выполнено: '+$_.Exception.Message)}
finally{
 if($stage -and(Test-Path -LiteralPath $stage)){Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue}
 if($locked){$mutex.ReleaseMutex()};if($mutex){$mutex.Dispose()}
}
