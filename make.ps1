cmake --build ./Build --target uieditor -j 6
write-host $lastExitCode
# write-host $args[0]

if (( $args[0] -eq "run" ) -and ($lastExitCode -eq 0)) {
    start-process ./bin/uieditor.exe
}
