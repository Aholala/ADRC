param([string]$Compiler='C:\ST\STM32CubeCLT_1.20.0\GNU-tools-for-STM32\bin\arm-none-eabi-g++.exe')
$ErrorActionPreference='Stop'
$adrcRepo=Split-Path -Parent $PSScriptRoot
$adrcSources=@('adrc')
$adrcTargets=@(@{Name='m4';Cpu='cortex-m4';Fpu='fpv4-sp-d16'},@{Name='m7';Cpu='cortex-m7';Fpu='fpv5-d16'})
$adrcAr=Join-Path (Split-Path -Parent $Compiler) 'arm-none-eabi-ar.exe'
if (!(Test-Path -LiteralPath $Compiler) -or !(Test-Path -LiteralPath $adrcAr)) { throw 'ARM compiler/ar not found' }
$adrcResults=@()
foreach($adrcTarget in $adrcTargets) {
    $adrcOut=Join-Path $adrcRepo "build/arm/$($adrcTarget.Name)"
    New-Item -ItemType Directory -Path $adrcOut -Force | Out-Null
    $adrcObjects=@()
    foreach($adrcSource in $adrcSources) {
        $adrcObject=Join-Path $adrcOut "$adrcSource.o"
        & $Compiler -std=c++11 -O2 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-exceptions -fno-rtti -mthumb -mfloat-abi=hard "-mcpu=$($adrcTarget.Cpu)" "-mfpu=$($adrcTarget.Fpu)" -I (Join-Path $adrcRepo 'include') -c (Join-Path $adrcRepo "src/$adrcSource.cpp") -o $adrcObject
        if($LASTEXITCODE -ne 0) { throw "ARM compilation failed: $adrcSource" }
        $adrcObjects+=$adrcObject
    }
    $adrcArchive=Join-Path $adrcOut 'libadrc.a'
    # Recreate the archive so renamed sources cannot leave obsolete members.
    if(Test-Path -LiteralPath $adrcArchive) {Remove-Item -LiteralPath $adrcArchive}
    & $adrcAr rcs $adrcArchive @adrcObjects
    if($LASTEXITCODE -ne 0) { throw 'ARM archive failed' }
    $adrcResults+=@{cpu=$adrcTarget.Cpu;fpu=$adrcTarget.Fpu;float_abi='hard';objects=$adrcObjects.Count;archive=$adrcArchive;bytes=(Get-Item -LiteralPath $adrcArchive).Length;passed=$true}
    Write-Output "ARM_LIBRARY_PASS $($adrcTarget.Name): $($adrcObjects.Count) objects"
}
$adrcHashes=@()
foreach($adrcSource in $adrcSources) { $adrcHashes+=Get-FileHash -LiteralPath (Join-Path $adrcRepo "src/$adrcSource.cpp") -Algorithm SHA256 | Select-Object Path,Hash }
foreach($adrcHeader in (Get-ChildItem -LiteralPath (Join-Path $adrcRepo 'include/adrc') -Filter '*.hpp')) { $adrcHashes+=Get-FileHash -LiteralPath $adrcHeader.FullName -Algorithm SHA256 | Select-Object Path,Hash }
$adrcReport=@{compiler=$Compiler;compiler_version=(& $Compiler --version | Select-Object -First 1);targets=$adrcResults;hashes=$adrcHashes;firmware_elf_linked=$false;hardware_tested=$false;flags='C++11 -O2 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -fno-exceptions -fno-rtti -mthumb -mfloat-abi=hard'}
$adrcReport | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $adrcRepo 'build/arm/verification.json') -Encoding UTF8
