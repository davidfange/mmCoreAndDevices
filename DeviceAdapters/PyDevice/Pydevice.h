///////////////////////////////////////////////////////////////////////////////
// FILE:          Pydevice.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Generic device adapter that runs a Python script. Serves as base class for PyCamera, etc.
//                
// AUTHOR:        Ivo Vellekoop
//                Jeroen Doornbos
//
// COPYRIGHT:     University of Twente, Enschede, The Netherlands.
//
// LICENSE:       This file is distributed under the BSD license.
//                License text is included with the source distribution.
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.

#ifndef _Pydevice_H_
#define _Pydevice_H_

#include <string>
#include <DeviceBase.h>
#include "PythonBridge.h"


/**
 * Base class for device adapters that are implement by a Python script.
 * Note: the MM API uses the Curiously Recurring Template pattern (antipattern). This strongly complicates everything. This is the reason for the class parameter T, the 'this->' prefixes, and the fact that all methods are declared in the header file.  
 * @tparam T Base type to implement. Should be CCameraBase<PythonBridge>, CGenericDevice<PythonBridge>, etc.
*/
template <class T>
class CPyDeviceBase : public T
{
protected:
    /** Object implementing all Python connectivity */
    PythonBridge python_;       
    
    /** Name of the adapter type, for use in GetName only */
    const char* adapterName_;   
public:
    /**
     * Constructs a new device
     * The device is not initialized, and no Python calls are made. This only sets up error messages, the error handler, and three 'pre-init' properties that hold the path of the Python libraries, the path of the Python script, and the name of the Python class that implements the device.
     * @param adapterName name of the adapter type, e.g. "PyCamera"
    */
    CPyDeviceBase(const char* adapterName) : adapterName_(adapterName), python_([this](const char* message) {
        this->SetErrorText(ERR_PYTHON_EXCEPTION, message);
        this->LogMessage(message, false);
    }) {
        this->SetErrorText(ERR_PYTHON_NOT_FOUND, "Could not initialize python interpreter, perhaps an incorrect path was specified?");
        this->SetErrorText(ERR_PYTHON_PATH_CONFLICT, "All Python devices must have the same Python library path");
        this->SetErrorText(ERR_PYTHON_SCRIPT_NOT_FOUND, "Could not find the python script at the specified location");
        this->SetErrorText(ERR_PYTHON_CLASS_NOT_FOUND, "Could not find a class definition with the specified name");
        this->SetErrorText(ERR_PYTHON_EXCEPTION, "The Python code threw an exception, check the CoreLog error log for details");
        this->SetErrorText(ERR_PYTHON_NO_INFO, "A Python error occurred, but no further information was available");
        this->SetErrorText(ERR_PYTHON_MISSING_PROPERTY, "The Python class is missing a required property, check CoreLog error log for details");

        python_.Construct(this);
    }
    virtual ~CPyDeviceBase() {
    }

    /**
     * Executes the Python script and creates a Python object corresponding to the device
     * Initializes the Python interpreter (if needed).
     * The Python class may perform hardware initialization in its __init__ function. After creating the Python object and initializing it, the function 'InitializeDevice' is called, which may be overridden e.g. to check if all required properties are present on the Python object (see PyCamera for an example).
     * @return MM error code 
    */
    int Initialize() override {
        auto result = python_.Initialize(this);
        if (result != DEVICE_OK) {
            Shutdown();
            return result;
        }
        result = InitializeDevice();
        if (result != DEVICE_OK) {
            Shutdown();
            return result;
        }
        return DEVICE_OK;
    }

    /**
     * Destroys the Python object
     * @todo Currently, the Python interperter is never de-initialized, even if all devices have been destroyed.
    */
    int Shutdown() override {
        return python_.Destruct();
    }

    /**
     * Required by MM::Device API. Returns name of the adapter type
    */
    void GetName(char* name) const override {
        CDeviceUtils::CopyLimitedString(name, adapterName_);
    }

protected:
    /**
    * Called after construction of the Python class
    * May be overridden by a derived class to check if all required properties are present and have the correct type,
    * or to perform any other initialization if needed.
    */
    virtual int InitializeDevice() {
        return DEVICE_OK;
    }
};

/**
 * Class representing a generic device that is implemented by Python code
 * @todo add buttons to the GUI so that we can activate the device so that it actually does something
*/
class CPyGenericDevice : public CPyDeviceBase<CGenericBase<PythonBridge>> {
    using BaseClass = CPyDeviceBase<CGenericBase<PythonBridge>>;
public:
    constexpr static const char* g_adapterName = "PyDevice";
    CPyGenericDevice() : BaseClass(g_adapterName) {
    }
    virtual bool Busy() override {
        return false;
    }
};

class CPyCamera : public CPyDeviceBase<CCameraBase<PythonBridge>> {
    /** numpy array corresponding to the last image, we hold a reference count so that we are sure the array does not get deleted during processing */
    PyObj lastImage_;       
    
    /** 'trigger' function of the camera object */
    PyObj triggerFunction_;
    PyObj waitFunction_;    // 'wait' function of the camera object
    
    using BaseClass = CPyDeviceBase<CCameraBase<PythonBridge>>;
public:
    constexpr static const char* g_adapterName = "PyCamera";
    CPyCamera() : BaseClass(g_adapterName) {
    }
    const unsigned char* GetImageBuffer() override;
    unsigned GetImageWidth() const override;
    unsigned GetImageHeight() const override;
    unsigned GetImageBytesPerPixel() const override;
    unsigned GetBitDepth() const override;
    long GetImageBufferSize() const override;
    int SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize) override;
    int GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize) override;
    int ClearROI() override;
    double GetExposure() const override;
    void SetExposure(double exp) override;
    int GetBinning() const override;
    int SetBinning(int binF) override;
    int IsExposureSequenceable(bool& isSequenceable) const override;
    int SnapImage() override;
protected:
    int InitializeDevice() override;
};
#endif //_Pydevice_H_
