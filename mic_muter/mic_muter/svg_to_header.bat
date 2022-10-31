@echo off
echo %1 -> %2
@(
    echo const char *%~n1_svg = R"***(
    type %~1
    echo ^)***";
) > %~2
