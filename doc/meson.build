doxygen = find_program('doxygen', required: true)


doxygen_version = run_command(doxygen, '--version').stdout().strip()
if doxygen_version.version_compare('< 1.8')
    error('Use at least doxygen version 1.8, found ' + doxygen_version)
endif


doxygen_output_dir = 'doxygen_doc'

doxy_conf_data = configuration_data()
doxy_conf_data.set('PROJECT_NAME', meson.project_name())
doxy_conf_data.set('PROJECT_VERSION', meson.project_version())
doxy_conf_data.set('SRC_ROOT', meson.source_root())
doxy_conf_data.set('CUR_SOURCE_DIR', meson.current_source_dir())
doxy_conf_data.set('OUTPUT_DIR', meson.build_root() + '/' + doxygen_output_dir)
doxy_conf = configure_file(
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: doxy_conf_data
)

custom_target(
    'doxygen_doc',
    command: [doxygen, doxy_conf],
    build_always_stale: true,
    output: doxygen_output_dir,
)
