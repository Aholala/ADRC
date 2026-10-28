param([string]$Compiler=$env:ADRC_CXX)
$ErrorActionPreference='Stop'
$adrcRepo=Split-Path -Parent $PSScriptRoot
if(!$Compiler) {
    $adrcFound=Get-Command g++ -ErrorAction SilentlyContinue
    if($adrcFound) {$Compiler=$adrcFound.Source} else {$Compiler='D:\msys64\ucrt64\bin\g++.exe'}
}
if(!(Test-Path -LiteralPath $Compiler)) {throw 'Set ADRC_CXX or -Compiler to the full g++ path'}
$adrcCmake=(Get-Command cmake -ErrorAction Stop).Source
$adrcNinja=(Get-Command ninja -ErrorAction Stop).Source
$adrcCtest=Join-Path (Split-Path -Parent $adrcCmake) 'ctest.exe'
$adrcBuild=Join-Path $adrcRepo 'build/host'
$adrcSavedPath=$env:PATH
try {
    $env:PATH=(Split-Path -Parent $Compiler)+';'+$env:PATH
    & $adrcCmake -S $adrcRepo -B $adrcBuild -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DADRC_BUILD_EXAMPLES=ON "-DCMAKE_CXX_COMPILER=$Compiler" "-DCMAKE_MAKE_PROGRAM=$adrcNinja"
    if($LASTEXITCODE -ne 0) {throw 'Host configure failed'}
    & $adrcCmake --build $adrcBuild --parallel 2
    if($LASTEXITCODE -ne 0) {throw 'Host build failed'}
    $adrcOutput=& $adrcCtest --test-dir $adrcBuild --output-on-failure
    $adrcStatus=$LASTEXITCODE
    $adrcOutput | Write-Output
    if($adrcStatus -ne 0) {throw 'Host tests failed'}
    $adrcText=$adrcOutput -join "`n"
    $adrcCount=[regex]::Match($adrcText,'out of (\d+)').Groups[1].Value
    if(!$adrcCount) {throw 'Could not confirm the test count'}
    $adrcReport=@{passed=$true;test_count=[int]$adrcCount;compiler=$Compiler;compiler_version=(& $Compiler --version | Select-Object -First 1);cxx_standard=11;output=$adrcText;hardware_tested=$false}
    $adrcReport | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $adrcBuild 'host_verification.json') -Encoding UTF8
} finally {$env:PATH=$adrcSavedPath}
