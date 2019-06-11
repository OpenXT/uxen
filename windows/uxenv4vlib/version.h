
#ifndef _VERSION_H_
#define _VERSION_H_

#ifdef BUILD_INFO
#include BUILD_INFO
#endif

#define	UXEN_DRIVER_VERSION_MAJOR	0x0000
#define	UXEN_DRIVER_VERSION_MINOR	0x0005
#define	UXEN_DRIVER_VERSION_REVISION	0x0000
#define	UXEN_DRIVER_VERSION_BUILD	0x0000

#define	UXEN_DRIVER_VERSION_TAG		"matricks"

#define UXEN_DRIVER_PRODUCTVERSION_STR "0.5 " UXEN_DRIVER_VERSION_TAG

#ifndef UXEN_DRIVER_VERSION_CHANGESET
#define UXEN_DRIVER_VERSION_CHANGESET "undefined"
#endif

#ifndef UXEN_DRIVER_FILEVERSION1
#define UXEN_DRIVER_FILEVERSION1 0x0
#endif
#ifndef UXEN_DRIVER_FILEVERSION2
#define UXEN_DRIVER_FILEVERSION2 0x0
#endif
#ifndef UXEN_DRIVER_FILEVERSION3
#define UXEN_DRIVER_FILEVERSION3 0x0
#endif
#ifndef UXEN_DRIVER_FILEVERSION4
#define UXEN_DRIVER_FILEVERSION4 0x0
#endif

#endif  /* _VERSION_H_ */
