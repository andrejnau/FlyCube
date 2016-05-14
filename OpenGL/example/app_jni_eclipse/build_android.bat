@echo off

call android update project -p . -t android-23
call ndk-build clean && (echo success) || (goto errorBuildFail)
call ndk-build && (echo success) || (goto errorBuildFail)
call ant debug && (echo success) || (goto errorBuildFail)

IF "%1" NEQ "install" goto noInstall

call ant installd

:noInstall
popd
goto end

:errorBuildFail
echo Error: the sample failed to build.
popd
exit /B 1

:end
@echo on