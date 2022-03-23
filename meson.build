project('hydra', 'cpp',
    license: 'MIT',
    version: '0.1.1',
    default_options: [
        'cpp_std=c++20',
        'warning_level=3'])

headers = [
    'include/hydra/activity.hpp',
    'include/hydra/batch.hpp',
    'include/hydra/mpsc_queue.hpp',
    'include/hydra/sequence.hpp',
    'include/hydra/spsc_queue.hpp'
]

incdirs = include_directories('./include')

hydra = declare_dependency(
    version: meson.project_version(),
    include_directories: incdirs,
    sources: headers
)

subdir('test')

install_headers(headers, subdir: 'hydra')

pkg = import('pkgconfig')
pkg.generate(
    name: 'hydra',
    filebase: 'hydra',
    subdirs: ['hydra'],
    version: meson.project_version(),
    description: 'C++17 header-only library for active objects'
)
