set /p Version=<last_commit

# sentry-cli difutil bundle-sources x64/Release/output.pdb
sentry-cli upload-dif -o slotted -p slotted-league x64/Release/output.pdb
sentry-cli releases set-commits --local %Version% -o slotted -p slotted-league
