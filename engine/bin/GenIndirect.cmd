.\tools\cmgen.exe -t equirect -f exr --sh=3 .\818-hdri-skies-com\818-hdri-skies-com.hdr

.\tools\cmgen.exe -t equirect -f exr --sh-output=.\818-hdri-skies-com\sky.exr .\818-hdri-skies-com\818-hdri-skies-com.hdr

.\tools\cmgen.exe -t equirect -f exr --sh-irradiance .\818-hdri-skies-com\818-hdri-skies-com.hdr


.\tools\cmgen.exe -t equirect -f exr --ibl-ld=. .\818-hdri-skies-com\818-hdri-skies-com.hdr

.\tools\cmgen.exe -t equirect -f exr --ibl-dfg=.\818-hdri-skies-com\ibldfg.exr .\818-hdri-skies-com\818-hdri-skies-com.hdr

