..\..\..\cmake-build\cmake\tools\cmgen\Debug\cmgen.exe -t equirect -f exr --sh=3 818-hdri-skies-com.hdr

..\..\..\cmake-build\cmake\tools\cmgen\Debug\cmgen.exe -t equirect -f exr --sh-output=sky.exr 818-hdri-skies-com.hdr

..\..\..\cmake-build\cmake\tools\cmgen\Debug\cmgen.exe -t equirect -f exr --sh-irradiance 818-hdri-skies-com.hdr


..\..\..\cmake-build\cmake\tools\cmgen\Debug\cmgen.exe -t equirect -f exr --ibl-ld=. 818-hdri-skies-com.hdr

..\..\..\cmake-build\cmake\tools\cmgen\Debug\cmgen.exe -t equirect -f exr --ibl-dfg=.\ibldfg.exr 818-hdri-skies-com.hdr

