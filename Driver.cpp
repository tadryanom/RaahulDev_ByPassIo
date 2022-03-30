/*
	Implementation of the bypassio filter driver to support on Windows 11.
	This is a very simple driver can be installed as upper as well as lower
	filter to classGuid.
	for now, I've taken it to support DiskDrive class devices.
*/

#include "Driver.h"

/*
  DriverEntry
		This routine is called when the driver is first loaded. Our job will be
		to create our WDF driver object.
  INPUTS:
		DriverObject - Address of the WDM driver object
		RegistryPath - Our driver's service key
  OUTPUTS:
		None.
  RETURNS:
		STATUS_SUCCESS. Otherwise an error indicating why the driver could not load.
  IRQL:
		PASSIVE_LEVEL.
*/

extern "C"
NTSTATUS
DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryEntry) 
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG config {};

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryEntry);

	KdPrint(("ByPassFilter Driver...Compiled %s %s\n", __DATE__, __TIME__));

	// Initializing the driver config structure, specifying the add device callback.
	WDF_DRIVER_CONFIG_INIT(&config, ByPassAddDevice);

	// No need to setup an unload entry point however this is the simplest driver so lets just keep it for refernce.
	config.EvtDriverUnload = DriverUnload;

	// Create our WDFDEVICE Object and hook-up with the Framework
	status = WdfDriverCreate(	DriverObject,
								RegistryEntry,
								WDF_NO_OBJECT_ATTRIBUTES,
								&config,
								WDF_NO_HANDLE);

	if (!NT_SUCCESS(status)) {
		KdPrint(("ByPassFilter : DriverEntry: WdfDriverCreate failed - 0x%x\n", status));
	}

	return status;
}

/*
  ByPassAddDevice
		This routine is called once for each instance of the type of device that we filter in the system.
		We will create a WDF filter device object and do any per device initialization.
  INPUTS:
		Driver     - Address of our WDF driver object
		DeviceInit - The WDFDEVICE_INIT structure that, once we're done
					 filling in, this will describe the properties of our filter device.
  OUTPUTS:
		DeviceInit - Since we may be filling in a lot of this structure,so technically this considered an output..
  RETURNS:
		Status code.
  IRQL:
		PASSIVE_LEVEL.
*/

NTSTATUS ByPassAddDevice(__in WDFDRIVER Driver, __in PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS                     status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES        ObjectAttr{};
	WDFDEVICE                    Device = nullptr;
	PBYPASS_DEVICE_CONTEXT       devContext = nullptr;
	WDF_IO_QUEUE_CONFIG          IoQueueConfig{};

	UNREFERENCED_PARAMETER(Driver);

	KdPrint(("ByPass Filter, Add device instance\n"));

	// Inform the framework that we are a filter driver.
	WdfFdoInitSetFilter(DeviceInit);

	// Define unnamed device type for now, since we are a generic kind of filter.
	// This doesn't matter much.
	WdfDeviceInitSetDeviceType(DeviceInit,FILE_DEVICE_UNKNOWN);

	// Now setup device attributes to our context type.
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ObjectAttr,BYPASS_DEVICE_CONTEXT);

	// Create our own wdf device.
	status = WdfDeviceCreate(&DeviceInit,
							 &ObjectAttr,
							 &Device);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceCreate failed with status = 0x%x\n", status));
		return status;
	}

	// Get the context, since we have created our device.
	devContext =  ByPassGetDeviceContext(Device);

	// Fill in the target for our IO requests.
	devContext->WdfDevice = Device;
	devContext->TargetWhereToSendRequest = WdfDeviceGetIoTarget(Device);
	devContext->TargetDevice = WdfIoTargetWdmGetTargetDeviceObject(WdfDeviceGetIoTarget(Device));

	KdPrint(("The target device for our reuest is %wZ ", devContext->TargetDevice->DriverObject->DriverName));

	// Now its time to create our default queue to handle IO requests.
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&IoQueueConfig, WdfIoQueueDispatchParallel);

	// Define callback to handle the IO. since for now we are only interested in 
	// IRP_MJ_DEVICE_CONTROL requests.

	IoQueueConfig.EvtIoDeviceControl = ByPassDeviceIoControl;

	IoQueueConfig.EvtIoInternalDeviceControl = ByPassInternalDeviceControl;

	IoQueueConfig.EvtIoRead = ByPassDispatchRead;

	IoQueueConfig.EvtIoWrite = ByPassDispatchWrite;

	// Now create the corresponding queue.
	status = WdfIoQueueCreate(	Device,
								&IoQueueConfig,
								WDF_NO_OBJECT_ATTRIBUTES,
								WDF_NO_HANDLE);
	if (!NT_SUCCESS(status)) {
		KdPrint(("ByPassAddDevice : WdfIoQueueCreate failed with status = 0x%x\n", status));
		return status;
	}

	return status;
}

void DriverUnload(__in WDFDRIVER Driver)
{
	UNREFERENCED_PARAMETER(Driver);
	KdPrint(("ByPass Driver is Unloading...\n"));

	return;
}



/*
  ByPassDeviceIoControl
		This routine is called when the device that we're filtering is sent a DEVICE IO CONTROL requests.
  INPUTS:
		Queue              - Our filter device's default WDF queue
		Request            - The WDF device control request.
		OutputBufferLength - Size of the output buffer, if any.
		InputBufferLength  - Size of the input buffer, if any.
		IoControlCode      - The operation.
  OUTPUTS:
		None.
  RETURNS:
		None.
  IRQL:
		Completely depends upon dveice, ~ <=DISPATCH_LEVEL.
*/

#pragma warning(disable : 4267)

void 
ByPassDeviceIoControl(	__in WDFQUEUE Queue,
						__in WDFREQUEST Request,
						__in size_t OutputBufferLength,
						__in size_t InputBufferLength,
						__in ULONG IoControlCode)
{
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	PBYPASS_DEVICE_CONTEXT devContext = nullptr;
	WDFDEVICE			   Device = nullptr;
	size_t				   bufferLength = 0;
	NTSTATUS			   status;

	Device = WdfIoQueueGetDevice(Queue);
	if (Device) {
		devContext = ByPassGetDeviceContext(Device);
	}

	switch (IoControlCode) {

	case IOCTL_STORAGE_MANAGE_BYPASS_IO: {

		KdPrint(("ByPassDeviceIoControl with I/O: "
			"command 0x%x, input buffer length %d, output buffer length %d \n",
			IoControlCode, (ULONG)InputBufferLength, (ULONG)OutputBufferLength));

		BPIO_INPUT* ByPassInRequest = nullptr;
		status = WdfRequestRetrieveInputBuffer( Request,
												0,
												(PVOID*)&ByPassInRequest,
												&bufferLength);
		if (!NT_SUCCESS(status))
			goto end;

		if (bufferLength < sizeof(BPIO_INPUT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("Input buffer is small so exit!!!\n"));
			break;
		}

		BPIO_OUTPUT* ByPassOutRequest = nullptr;
		status = WdfRequestRetrieveOutputBuffer(Request,
												0,
												(PVOID*)&ByPassOutRequest,
												&bufferLength);
		if (!NT_SUCCESS(status))
			goto end;

		if (bufferLength < sizeof(BPIO_OUTPUT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("Output buffer is small so exit!!!\n"));
			break;
		}

		if (ByPassInRequest->Operation == BPIO_OP_ENABLE) {
			ByPassOutRequest->Version = sizeof(BPIO_OUTPUT);
			ByPassOutRequest->Size = sizeof(BPIO_INPUT) + bufferLength;
			ByPassOutRequest->Operation = BPIO_OP_ENABLE;
			ByPassOutRequest->OutFlags = BPIO_OUTFL_NONE;
			ByPassOutRequest->Enable.OpStatus = STATUS_SUCCESS;
		}
		else if (ByPassInRequest->Operation == BPIO_OP_DISABLE) {
			ByPassOutRequest->Version = sizeof(BPIO_OUTPUT);
			ByPassOutRequest->Size = sizeof(BPIO_INPUT) + bufferLength;
			ByPassOutRequest->Operation = BPIO_OP_DISABLE;
			ByPassOutRequest->Enable.OpStatus = STATUS_SUCCESS;
		}
		else if (ByPassInRequest->Operation == BPIO_OP_QUERY) {
			ByPassOutRequest->Version = sizeof(BPIO_OUTPUT);
			ByPassOutRequest->Size = sizeof(BPIO_INPUT) + bufferLength;
			ByPassOutRequest->Operation = BPIO_OP_ENABLE;
			ByPassOutRequest->OutFlags = BPIO_OUTFL_NONE;
			ByPassOutRequest->Enable.OpStatus = STATUS_SUCCESS;
		}

		// Setup the request for the next driver
		WdfRequestFormatRequestUsingCurrentType(Request);

		//ByPassFilterSendWithCallback(Request, devContext);
		// Pass the request on to the filtered device and forget.
		ByPassForwardRequest(WdfIoQueueGetDevice(Queue), Request);

		return;
	}
	}
end:

	// Pass the request on to the filtered device and forget.
	ByPassForwardRequest(WdfIoQueueGetDevice(Queue), Request);

	return;
}

VOID
ByPassFilterSendWithCallback(WDFREQUEST                Request,
							 PBYPASS_DEVICE_CONTEXT DevContext)
{
	NTSTATUS status;

	KdPrint(("Sending %p with completion\n", Request));

	//
	// Setup the request for the next driver
	//
	WdfRequestFormatRequestUsingCurrentType(Request);

	//
	// Set the completion routine...
	//
	WdfRequestSetCompletionRoutine(Request,
		ByPassFilterCompletionCallback,
		DevContext);
	//
	// And send it!
	// 
	if (!WdfRequestSend(Request,
		WdfDeviceGetIoTarget(DevContext->WdfDevice),
		WDF_NO_SEND_OPTIONS)) {

		//
		// Oops! Something bad happened, complete the request
		//
		status = WdfRequestGetStatus(Request);

		KdPrint(("WdfRequestSend failed = 0x%x\n", status));
		WdfRequestComplete(Request,
			status);
	}

	//
	// When we return the Request is always "gone"
	//
}



VOID
ByPassFilterCompletionCallback(WDFREQUEST                     Request,
	WDFIOTARGET                    Target,
	PWDF_REQUEST_COMPLETION_PARAMS Params,
	WDFCONTEXT                     Context)
{
	NTSTATUS status;
	auto* devContext = (PBYPASS_DEVICE_CONTEXT)Context;

	UNREFERENCED_PARAMETER(Target);
	UNREFERENCED_PARAMETER(devContext);

	KdPrint(("ByPassFilterCompletionCallback: Request=%p, Status=0x%x; Information=0x%Ix\n",
		Request,
		Params->IoStatus.Status,
		Params->IoStatus.Information));

	status = Params->IoStatus.Status;

	//
	// Potentially do something interesting here
	//

	WdfRequestComplete(Request,
		status);
}


void
ByPassForwardRequest(__in WDFDEVICE Device,
					 __in WDFREQUEST Request)
{
	WDF_REQUEST_SEND_OPTIONS		options;
	PBYPASS_DEVICE_CONTEXT          devContext;
	NTSTATUS						status;

	// Get the context.
	devContext = ByPassGetDeviceContext(Device);
	
	//  use the WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET option
	WDF_REQUEST_SEND_OPTIONS_INIT(&options,
								  WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

	if (!WdfRequestSend(Request,
						WdfDeviceGetIoTarget(devContext->WdfDevice),
						&options))
	{
		status = WdfRequestGetStatus(Request);
		KdPrint(("ByPassForwardRequest : WdfRequestSend failed with status =  0x%x\n", status));
		WdfRequestComplete(Request, status);
	}

	return;
}


void 
ByPassInternalDeviceControl(__in WDFQUEUE Queue, 
							__in WDFREQUEST Request, 
							__in size_t OutputBufferLength,
							__in size_t InputBufferLength,
							__in ULONG IoControlCode)
{
	PIRP                    InternaldeviceControlIrp = nullptr;
	PBYPASS_DEVICE_CONTEXT  devContext = nullptr;
	WDFDEVICE			    Device = nullptr;

	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	Device = WdfIoQueueGetDevice(Queue);
	if (Device) {
		devContext = ByPassGetDeviceContext(Device);
	}



	// Get the WDM IRP
	InternaldeviceControlIrp = WdfRequestWdmGetIrp(Request);

	switch (IoControlCode) {

	case IOCTL_STORAGE_MANAGE_BYPASS_IO:
		KdPrint(("Application invoked IOCTL_STORAGE_MANAGE_BYPASS_IO\n"));


		KdPrint(("IRP-0x%p; InputBufferLength-%lld; OutputBufferLength-%lld; "\
			"Control Code-0x%x\n",
			InternaldeviceControlIrp, InputBufferLength, OutputBufferLength,
			IoControlCode));

		ByPassFilterSendWithCallback(Request, devContext);

		break;

	default:
		break;
	}

	ByPassForwardRequest(WdfIoQueueGetDevice(Queue), Request);
	return;
}


VOID
ByPassDispatchRead(WDFQUEUE   Queue,
	WDFREQUEST Request,
	size_t     Length)
{
	PBYPASS_DEVICE_CONTEXT devContext;

	UNREFERENCED_PARAMETER(Length);

	devContext = ByPassGetDeviceContext(WdfIoQueueGetDevice(Queue));

#if DBG
	DbgPrint("ByPassDispatchRead -- Request 0x%p\n",
		Request);
#endif
	// We want to see the results for this particular Request... so send it
	// and request a callback for when the Request has been completed.
	ByPassFilterSendWithCallback(Request,
		devContext);
}


VOID
ByPassDispatchWrite(WDFQUEUE   Queue,
	WDFREQUEST Request,
	size_t     Length)
{
	PBYPASS_DEVICE_CONTEXT devContext;

	UNREFERENCED_PARAMETER(Length);

	devContext = ByPassGetDeviceContext(WdfIoQueueGetDevice(Queue));

#if DBG
	DbgPrint("ByPassDispatchWrite -- Request 0x%p\n",
		Request);
#endif


	// We want to see the results for this particular Request... so send it
	// and request a callback for when the Request has been completed.
	ByPassFilterSendWithCallback(Request,
		devContext);
}


