rpicam_app_src += files([
    'circular_output.cpp',
    'file_output.cpp',
    'net_output.cpp',
    'output.cpp',
    'file_output_mjpeg.cpp'
])

output_headers = [
    'circular_output.hpp',
    'file_output.hpp',
    'net_output.hpp',
    'output.hpp',
    'file_output_mjpeg.hpp'
]

rpicam_app_dep += [exif_dep, jpeg_dep, tiff_dep, png_dep]

install_headers(files(output_headers), subdir: meson.project_name() / 'output')
