#include "secure_erase.h"

namespace Updater {
SecureErase &SecureErase::GetInstance()
{
    static SecureErase secureEraseInstance;
    return secureEraseInstance;
}

SecureErase::SecureErase() {}

void SecureErase::LoadOffsetInRetry(uint64_t offset)
{
}

void SecureErase::ShowCurrentPercent(float value)
{
}

void SecureErase::ShowRemainingTime(uint64_t remainSeconds)
{
    (void)remainingOverWriteTime_;
}

bool SecureErase::OverWritePartition()
{
    return true;
}

int SecureErase::OverWritePartition(int fd, const uint32_t writeSize, std::vector<uint8_t> &buffer)
{
    return 0;
}

void SecureErase::AddOverWritePartition(const std::string &devPath)
{
}

void SecureErase::AddOverWritePartInfo(const PartInfo &partInfo)
{
}

void SecureErase::SyncOffsetInMisc(uint64_t offset)
{
}

float SecureErase::CalcOverWriteProgress()
{
    (void)overwriteOffset_;
    return 0;
}
}
