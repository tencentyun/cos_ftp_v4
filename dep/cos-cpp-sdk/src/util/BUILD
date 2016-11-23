cc_library(
    name = 'util',
    srcs = [
        'CodecUtil.cpp',
        'FileUtil.cpp',
        'HttpSender.cpp',
        'HttpUtil.cpp',
        'l5_endpoint_provider.cpp',
        'Sha1.cpp',
        'StringUtil.cpp',
        'true_random.cpp',
	],
    deps = [
    ],
    incs = [
	    '//include/'
		'//include/curl'
		'//include/json'
		'//include/l5'
		'//include/op'
		'//include/request'
		'//include/thread'
		'//include/util'
    ],
    extra_cppflags = [
        '-pthread',
        '-fPIC',
        '-D_REENTRANT',
        '-D_GNU_SOURCE',
        '-Wno-sign-compare',
        '-fno-strict-aliasing',
    ]
)
