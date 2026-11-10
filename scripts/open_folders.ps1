Add-Type -AssemblyName System.Windows.Forms

$ModDir = "D:\SteamLibrary\steamapps\common\Judgment\runtime\media\mods\JEPlus"
$ProjectDir = "D:\DragonEngine\Projects\JEPlus"

Start-Process explorer $ModDir
Start-Sleep -Milliseconds 750

[System.Windows.Forms.SendKeys]::SendWait("^t")
Start-Sleep -Milliseconds 250
[System.Windows.Forms.SendKeys]::SendWait("^l")
Start-Sleep -Milliseconds 100
[System.Windows.Forms.SendKeys]::SendWait("$ProjectDir{ENTER}")
