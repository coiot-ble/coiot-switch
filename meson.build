project('app', 'C')

sdk = subproject('sdk',
    default_options: ['ble_dis=true'])
vanilla = subproject('vanilla',
    default_options: ['no_stderr=true','printf=NRF_LOG_INFO'])

app = files('main.c')

ldmap = '../app.ld'
e = executable(meson.project_name(), app,
	include_directories: sdk.get_variable('i') + vanilla.get_variable('vanilla_i'),
	link_with: [sdk.get_variable('l'), vanilla.get_variable('vanilla')],
	link_args: sdk.get_variable('link_args') + ['-T', ldmap],
	c_args: ['-Wall', '-Werror',
		'-DNRF_LOG_ENABLED', '-DNRF_LOG_DEFAULT_LEVEL=4',
		'-DSWI_DISABLE0', '-DBLE_STACK_SUPPORT_REQD',
		'-DSOFTDEVICE_PRESENT','-DS130','-DNRF_SD_BLE_API_VERSION=2'])

gcc_prefix = meson.get_cross_property('gcc_prefix', '')

objcopy=find_program(gcc_prefix+'objcopy')

hex=custom_target(meson.project_name() + '.hex',
	input: e,
	output: meson.project_name() + '.hex',
	command: [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
	build_always: true
)

run_target('flash',
	depends: hex,
	command: ['nrfjprog', '--program', hex, '--verify', '-f', 'nrf51', '--sectorerase', '-r']
)

run_target('flash_softdevice', command: sdk.get_variable('cmd_flash_softdevice'))
