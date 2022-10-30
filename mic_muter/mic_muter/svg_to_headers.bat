@echo off
for %%f in (images\\*.svg) do (
    echo %%~f
    @(
        echo const char *%%~nf_svg = R"***(
        type %%~f
        echo ^)***";
    ) > images\\%%~nf_svg.h
)
