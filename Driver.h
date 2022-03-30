#ifndef __DRIVER_H__
#define __DRIVER_H__

extern "C" {

#include <ntddk.h>
#include <wdf.h>
#include <ntddstor.h>

}

// This is per device context structure.

typedef struct _BYPASS_DEVICE_CONTEXT {

    WDFIOTARGET     TargetWhereToSendRequest;
    PDEVICE_OBJECT  TargetDevice;
    WDFDEVICE       WdfDevice;

} BYPASS_DEVICE_CONTEXT, * PBYPASS_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(BYPASS_DEVICE_CONTEXT, ByPassGetDeviceContext)


NTSTATUS
ByPassAddDevice(__in WDFDRIVER Driver,
                __in PWDFDEVICE_INIT  DeviceInit);

void
DriverUnload(__in WDFDRIVER Driver);

void
ByPassDeviceIoControl(  __in WDFQUEUE Queue,  
                        __in WDFREQUEST Request,
                        __in size_t OutputBufferLength,
                        __in size_t InputBufferLength,
                        __in ULONG IoControlCode);

void
ByPassForwardRequest(__in WDFDEVICE Device,
                     __in WDFREQUEST Request);


void
ByPassInternalDeviceControl(__in WDFQUEUE Queue,
                            __in WDFREQUEST Request,
                            __in size_t OutputBufferLength,
                            __in size_t InputBufferLength,
                            __in ULONG IoControlCode);

VOID
ByPassFilterSendWithCallback(WDFREQUEST                Request,
    PBYPASS_DEVICE_CONTEXT DevContext);

VOID
ByPassFilterCompletionCallback(WDFREQUEST                     Request,
    WDFIOTARGET                    Target,
    PWDF_REQUEST_COMPLETION_PARAMS Params,
    WDFCONTEXT                     Context);


VOID
ByPassDispatchWrite(WDFQUEUE   Queue,
    WDFREQUEST Request,
    size_t     Length);


VOID
ByPassDispatchRead(WDFQUEUE   Queue,
    WDFREQUEST Request,
    size_t     Length);

#endif