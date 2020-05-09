#include "Semaphore/Semaphore.h"

class DXDevice;

class DXSemaphore : public Semaphore
{
public:
    DXSemaphore(DXDevice& device);

private:
    DXDevice& m_device;
};
