cc_library(
    name = 'request',
    srcs = [
        'CosResult.cpp',
        'FileDeleteReq.cpp',
        'FileDownloadReq.cpp',
        'FileMoveReq.cpp',
		'FileReq.cpp',
		'FileStatReq.cpp',
		'FileUpdateReq.cpp',
		'FileUploadReq.cpp',
		'FolderCreateReq.cpp',
		'FolderDeleteReq.cpp',
		'FolderListReq.cpp',
		'FolderReq.cpp',
		'FolderStatReq.cpp',
		'FolderUpdateReq.cpp',
		'ReqBase.cpp',
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
