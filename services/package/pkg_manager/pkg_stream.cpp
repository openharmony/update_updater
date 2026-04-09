/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pkg_stream.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif
#include <unistd.h>
#include <cstdio>
#include "dump.h"
#include "pkg_manager.h"
#include "pkg_utils.h"
#include "ring_buffer/ring_buffer.h"
#include "securec.h"

#ifdef __APPLE__
#define off64_t off_t
#define fopen64 fopen
#define ftello64 ftello
#define fseeko64 fseek
#endif
using namespace Updater;
namespace Hpackage {
const std::string PkgStreamImpl::GetFileName() const
{
    return fileName_;
}

PkgStreamPtr PkgStreamImpl::ConvertPkgStream(PkgManager::StreamPtr stream)
{
    return (PkgStreamPtr)stream;
}

void PkgStreamImpl::AddRef()
{
    refCount_++;
}

void PkgStreamImpl::DelRef()
{
    refCount_--;
}

bool PkgStreamImpl::IsRef() const
{
    return refCount_ == 0;
}

void PkgStreamImpl::PostDecodeProgress(int type, size_t writeDataLen, const void *context) const
{
    if (pkgManager_ != nullptr) {
        pkgManager_->PostDecodeProgress(type, writeDataLen, context);
    }
}

FileStream::~FileStream()
{
    if (stream_ != nullptr) {
        fflush(stream_);
        fclose(stream_);
        stream_ = nullptr;
    }
}

int32_t FileStream::Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen)
{
    Updater::UPDATER_INIT_RECORD;
    std::lock_guard<std::recursive_mutex> lock(fileStreamLock_);
    if (stream_ == nullptr) {
        PKG_LOGE("Invalid stream");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "Invalid stream");
        return PKG_INVALID_STREAM;
    }
    if (data.length < needRead) {
        PKG_LOGE("insufficient buffer capacity");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "insufficient buffer capacity");
        return PKG_INVALID_STREAM;
    }
    readLen = 0;
    if (fseeko64(stream_, start, SEEK_SET) != 0) {
        PKG_LOGE("read data fail");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "fseeko64 fail");
        return PKG_INVALID_STREAM;
    }
    if (start > GetFileLength()) {
        PKG_LOGE("Invalid start");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "Invalid start");
        return PKG_INVALID_STREAM;
    }
    if (data.buffer == nullptr) {
        data.data.resize(data.length);
        data.buffer = data.data.data();
    }
    readLen = fread(data.buffer, 1, needRead, stream_);
    if (readLen == 0) {
        PKG_LOGE("read data fail %d", errno);
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "read data fail");
        return PKG_INVALID_STREAM;
    }
    return PKG_SUCCESS;
}

int64_t FileStream::GetReadOffset() const
{
    off64_t ret = ftello64(stream_);
    if (ret < 0) {
        PKG_LOGE("ftell64 failed %d", errno);
    }
    return static_cast<int64_t>(ret);
}

int32_t FileStream::Write(const PkgBuffer &data, size_t size, size_t start)
{
    std::lock_guard<std::recursive_mutex> lock(fileStreamLock_);
    if (streamType_ != PkgStreamType_Write) {
        PKG_LOGE("Invalid stream type");
        return PKG_INVALID_STREAM;
    }
    if (stream_ == nullptr) {
        PKG_LOGE("Invalid stream");
        return PKG_INVALID_STREAM;
    }
    if (fseeko64(stream_, start, SEEK_SET) != 0) {
        PKG_LOGE("write data fail");
        return PKG_INVALID_STREAM;
    }
    size_t len = fwrite(data.buffer, size, 1, stream_);
    if (len != 1) {
        PKG_LOGE("Write buffer fail");
        return PKG_INVALID_STREAM;
    }
    PostDecodeProgress(POST_TYPE_DECODE_PKG, size, nullptr);
    return PKG_SUCCESS;
}

size_t FileStream::GetFileLength()
{
    std::lock_guard<std::recursive_mutex> lock(fileStreamLock_);
    if (stream_ == nullptr) {
        PKG_LOGE("Invalid stream");
        return 0;
    }
    if (fileLength_ == 0) {
        if (Seek(0, SEEK_END) != 0) {
            PKG_LOGE("Invalid stream");
            return 0;
        }
        off64_t ret = ftello64(stream_);
        if (ret < 0) {
            PKG_LOGE("ftell64 failed");
            return 0;
        }
        fileLength_ = static_cast<size_t>(ret);
        if (fseek(stream_, 0, SEEK_SET) != 0) {
            PKG_LOGE("fseek failed");
            return 0;
        }
    }
    return fileLength_;
}

int32_t FileStream::Seek(long int offset, int whence)
{
    std::lock_guard<std::recursive_mutex> lock(fileStreamLock_);
    if (stream_ == nullptr) {
        PKG_LOGE("Invalid stream");
        return PKG_INVALID_STREAM;
    }
    return fseek(stream_, offset, whence);
}

int32_t FileStream::Flush(size_t size)
{
    std::lock_guard<std::recursive_mutex> lock(fileStreamLock_);
    if (stream_ == nullptr) {
        PKG_LOGE("Invalid stream");
        return PKG_INVALID_STREAM;
    }
    if (fileLength_ == 0) {
        fileLength_ = size;
    }
    if (fseek(stream_, 0, SEEK_END) != 0) {
        PKG_LOGE("fseek failed");
        return PKG_INVALID_STREAM;
    }
    off64_t ret = ftello64(stream_);
    if (ret < 0) {
        PKG_LOGE("ftell64 failed");
        return PKG_INVALID_STREAM;
    }
    fileLength_ = static_cast<size_t>(ret);
    if (size != fileLength_) {
        PKG_LOGE("Flush size %zu local size:%zu", size, fileLength_);
    }
    if (fflush(stream_) != 0) {
        PKG_LOGE("Invalid stream");
        return PKG_INVALID_STREAM;
    }
    return PKG_SUCCESS;
}

#ifndef DIFF_PATCH_SDK
int32_t ShmDataStream::CreateShmRingBuffer()
{
    PKG_LOGI("CreateShmRingBuffer");
    int fd = shm_open(shmId_.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        PKG_LOGE("shm_open failed");
        return -1;
    };
    ftruncate(fd, sizeof(ShmRingBuffer));
    // 映射内存空间
    rb_ = static_cast<ShmRingBuffer*>(mmap(nullptr, sizeof(ShmRingBuffer), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0));
    close(fd);
    if (rb_ == MAP_FAILED) {
        PKG_LOGE("ShmRingBuffer mmap failed");
        return -1;
    }

    // 共享内存控制变量初始化
    sem_init(&rb_->sem_empty, 1, BLOCK_NUM);
    sem_init(&rb_->sem_full, 1, 0);
    rb_->head = 0;
    rb_->tail = 0;
    rb_->currLen = 0;
    rb_->currOffset = 0;
    return 0;
}

int32_t ShmDataStream::InitShmRingBuffer()
{
    int fd = shm_open(shmId_.c_str(), O_RDWR, 0666);
    if (fd == -1) {
        PKG_LOGE("shm_open failed");
        return -1;
    };
    rb_ = static_cast<ShmRingBuffer*>(mmap(nullptr, sizeof(ShmRingBuffer), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0));
    close(fd);
    if (rb_ == MAP_FAILED) {
        PKG_LOGE("ShmRingBuffer mmap failed");
        return -1;
    }
    return 0;
}

int32_t ShmDataStream::ReadFully(size_t &needReadLen, size_t &readLen, PkgBuffer &data)
{
    if (rb_ == nullptr) {
        PKG_LOGE("rb is nullptr");
        return PKG_INVALID_PARAM;
    }
    while (needReadLen > 0) {
        sem_wait(&rb_->sem_full);

        size_t len = rb_->efficientLen[rb_->head];  // 块实际长度
        if (len == 0) {
            PKG_LOGE("efficient len invalid: %d", rb_->head);
            return PKG_INVALID_STREAM;
        }
        size_t offset = rb_->head * SINGLE_BLOCK_SIZE;
        if (needReadLen >= len) {  // 取一个满块
            memcpy_s(data.buffer + readLen, len, rb_->buffer + offset, len);
            rb_->efficientLen[rb_->head] = 0;
            needReadLen -= len;
            readLen += len;
            rb_->head = (rb_->head + 1) % BLOCK_NUM;
 
            sem_post(&rb_->sem_empty);
            continue;
        }

        // 取部分块
        // rb_->head ~ rb_->head + needReadLen copy to dest
        memcpy_s(data.buffer + readLen, needReadLen, rb_->buffer + offset, needReadLen);
        // rb_->head + needReadLen ~ rb_->head + block_len copy to temp buffer
        memcpy_s(rb_->reserved, len - needReadLen, rb_->buffer + offset + needReadLen, len - needReadLen);
        rb_->currLen = len - needReadLen;
        rb_->currOffset = 0;
        readLen += needReadLen;
        needReadLen -= needReadLen;
        rb_->efficientLen[rb_->head] = 0;
        rb_->head = (rb_->head + 1) % BLOCK_NUM;
 
        sem_post(&rb_->sem_empty);
    }
    return PKG_SUCCESS;
}

int32_t ShmDataStream::Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen)
{
    if (start != offset_) {
        PKG_LOGE("offset_ not matched");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "offset_ not matched");
        return PKG_INVALID_STREAM;
    }
    if (data.length < needRead) {
        PKG_LOGE("insufficient buffer capacity");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "insufficient buffer capacity");
        return PKG_INVALID_STREAM;
    }
    if (data.buffer == nullptr) {
        data.data.resize(needRead);
        data.buffer = data.data.data();
    }
    if (rb_ == nullptr) {
        PKG_LOGE("rb_ is nullptr");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "rb_ is nullptr");
        return PKG_INVALID_STREAM;
    }

    readLen = 0;
    size_t needReadLen = needRead;
    if (rb_->currLen > 0) {
        if (rb_->currLen >= needReadLen) {
            memcpy_s(data.buffer, needReadLen, rb_->reserved + rb_->currOffset, needReadLen);
            rb_->currOffset += needReadLen;
            rb_->currLen -= needReadLen;
            readLen = needReadLen;
            offset_ += readLen;
            return 0;
        }
        memcpy_s(data.buffer, rb_->currLen, rb_->reserved + rb_->currOffset, rb_->currLen);
        needReadLen -= rb_->currLen;
        readLen += rb_->currLen;
        rb_->currLen = 0;
        rb_->currOffset = 0;
    }
    if (ReadFully(needReadLen, readLen, data) != PKG_SUCCESS) {
        return PKG_INVALID_STREAM;
    }
    offset_ += readLen;
    return 0;
}

int32_t ShmDataStream::Write(const PkgBuffer &data, size_t size, size_t start)
{
    if (start != offset_) {
        PKG_LOGE("offset_ not matched");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "offset_ not matched");
        return PKG_INVALID_STREAM;
    }
    if (data.length < size || data.buffer == nullptr) {
        PKG_LOGE("insufficient buffer capacity");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "insufficient buffer capacity");
        return PKG_INVALID_STREAM;
    }
    if (rb_ == nullptr) {
        PKG_LOGE("rb_ is nullptr");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "rb_ is nullptr");
        return PKG_INVALID_STREAM;
    }
    size_t needWriteLen = size;
    size_t writedLen = 0;
    while (needWriteLen > 0) {
        sem_wait(&rb_->sem_empty);
 
        size_t len = needWriteLen < SINGLE_BLOCK_SIZE ? needWriteLen : SINGLE_BLOCK_SIZE;
        size_t offset = rb_->tail * SINGLE_BLOCK_SIZE;
        memcpy_s(rb_->buffer + offset, len, data.buffer + writedLen, len);
        rb_->efficientLen[rb_->tail] = len;
        rb_->tail = (rb_->tail + 1) % BLOCK_NUM;
        needWriteLen -= len;
        writedLen += len;
 
        sem_post(&rb_->sem_full);
    }

    offset_ += size;
    return 0;
}

void ShmDataStream::Stop()
{
    if (rb_ == nullptr) {
        PKG_LOGE("rb_ is nullptr");
        return;
    }
    if (munmap(rb_, sizeof(RingBuffer)) != 0) {
        PKG_LOGE("munmap failed : %s", strerror(errno));
        return;
    }
    rb_ = nullptr;
}

void ShmDataStream::Exit()
{
    if (rb_ == nullptr) {
        PKG_LOGE("rb_ is nullptr");
        return;
    }
    sem_destroy(&rb_->sem_empty);
    sem_destroy(&rb_->sem_full);
    if (munmap(rb_, sizeof(RingBuffer)) != 0) {
        PKG_LOGE("munmap failed : %s", strerror(errno));
        return;
    }
    rb_ = nullptr;
    if (shm_unlink(shmId_.c_str()) != 0) {
        PKG_LOGE("shm_unlink failed : %s", strerror(errno));
    }
}
#endif

#ifndef _WIN32
bool ShmRbBlock::Push(const uint8_t* buf, size_t len)
{
    if (buf == nullptr) {
        PKG_LOGE("Push error : buf is nullptr");
        return false;
    }
    if (len == 0 || len > SINGLE_BLOCK_SIZE) {
        PKG_LOGE("Push error : invalid len : %zu", len);
        return false;
    }
    
    if (memcpy_s(data_, SINGLE_BLOCK_SIZE, buf, len) != EOK) {
        PKG_LOGE("memcpy_s error, len : %zu", len);
        return false;
    }
    realLen_ = len;
    blkOffset_ = 0;
    return true;
}

bool ShmRbBlock::Pop(uint8_t* buf, size_t expectedLen, size_t &realLen)
{
    if (buf == nullptr) {
        PKG_LOGE("Pop error : buf is nullptr");
        return false;
    }
    if (expectedLen == 0 || expectedLen > SINGLE_BLOCK_SIZE) {
        PKG_LOGE("Pop error : invalid expectedLen : %zu", expectedLen);
        return false;
    }

    realLen = expectedLen < realLen_ ? expectedLen : realLen_;
    if (memcpy_s(buf, expectedLen, data_ + blkOffset_, realLen) != EOK) {
        PKG_LOGE("memcpy_s error, expectedLen : %zu, realLen : %zu", expectedLen, realLen);
        return false;
    }
    realLen_ -= realLen;
    blkOffset_ += realLen;
    return true;
}

size_t ShmRbBlock::GetRealLen()
{
    return realLen_;
}

int32_t ShmDataStream::CreateShmRingBuffer()
{
    int fd = shm_open(shmId_.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        PKG_LOGE("shm_open failed, errno = %d %s", errno, strerror(errno));
        return PKG_INVALID_STREAM;
    }
    int ret = ftruncate(fd, sizeof(ShmRingBuffer));
    if (ret < 0) {
        PKG_LOGE("ftruncate failed : error = %d %s", errno, strerror(errno));
        close(fd);
        return PKG_INVALID_STREAM;
    }
    // 映射内存空间
    if (rb_ != nullptr) {
        Stop();
    }
    rb_ = static_cast<ShmRingBuffer*>(mmap(nullptr, sizeof(ShmRingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    if (rb_ == MAP_FAILED) {
        PKG_LOGE("ShmRingBuffer mmap failed, error = %d %s", errno, strerror(errno));
        return PKG_INVALID_STREAM;
    }

    // 共享内存控制变量初始化
    ret = sem_init(&rb_->semEmpty, 1, BLOCK_NUM);
    if (ret != 0) {
        PKG_LOGE("ret : %d, error = %d %s", ret, errno, strerror(errno));
        return PKG_INVALID_STREAM;
    }
    ret = sem_init(&rb_->semFull, 1, 0);
    if (ret != 0) {
        PKG_LOGE("ret : %d, error = %d %s", ret, errno, strerror(errno));
        return PKG_INVALID_STREAM;
    }
    rb_->head = 0;
    rb_->tail = 0;
    PKG_LOGI("CreateShmRingBuffer success");
    return PKG_SUCCESS;
}

int32_t ShmDataStream::InitShmRingBuffer()
{
    int fd = shm_open(shmId_.c_str(), O_RDWR, 0);
    if (fd < 0) {
        PKG_LOGE("shm_open failed, errno = %d %s", errno, strerror(errno));
        return PKG_INVALID_STREAM;
    }
    // 映射内存空间
    if (rb_ != nullptr) {
        Stop();
    }
    rb_ = static_cast<ShmRingBuffer*>(mmap(nullptr, sizeof(ShmRingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    if (rb_ == MAP_FAILED) {
        PKG_LOGE("ShmRingBuffer mmap failed, error = %d %s", errno, strerror(errno));
        return PKG_INVALID_STREAM;
    }
    PKG_LOGI("InitShmRingBuffer success");
    return PKG_SUCCESS;
}

int32_t ShmDataStream::Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen)
{
    if (start != offset_) {
        PKG_LOGE("offset_ not matched, start : %zu, offset : %zu", start, offset_);
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "offset_ not matched");
        return PKG_INVALID_STREAM;
    }
    if (data.length < needRead) {
        PKG_LOGE("insufficient buffer capacity");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "insufficient buffer capacity");
        return PKG_INVALID_STREAM;
    }
    if (data.buffer == nullptr) {
        data.data.resize(needRead);
        data.buffer = data.data.data();
    }
    if (rb_ == nullptr) {
        PKG_LOGE("rb_ is nullptr");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "rb_ is nullptr");
        return PKG_INVALID_STREAM;
    }

    readLen = 0;
    size_t needReadLen = needRead;
    while (needReadLen > 0) {
        sem_wait(&rb_->semFull);
        
        size_t realLen = 0;
        size_t readingLen = needReadLen < SINGLE_BLOCK_SIZE ? needReadLen : SINGLE_BLOCK_SIZE;
        if (!(rb_->data[rb_->head].Pop(data.buffer + readLen, readingLen, realLen))) {
            PKG_LOGE("pop error, start : %zu, needRead : %zu", start, needRead);
            return PKG_INVALID_STREAM;
        }
        needReadLen -= realLen;
        readLen += realLen;

        if (rb_->data[rb_->head].GetRealLen() > 0) {
            sem_post(&rb_->semFull); // 信号量回弹
            continue;
        }
        rb_->head = (rb_->head + 1) % BLOCK_NUM;
        sem_post(&rb_->semEmpty);
    }

    offset_ += readLen;
    return PKG_SUCCESS;
}

int32_t ShmDataStream::Write(const PkgBuffer &data, size_t size, size_t start)
{
    if (start != offset_) {
        PKG_LOGE("offset_ not matched, start : %zu, offset : %zu", start, offset_);
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "offset_ not matched");
        return PKG_INVALID_STREAM;
    }
    if (data.length < size || data.buffer == nullptr) {
        PKG_LOGE("insufficient buffer capacity");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "insufficient buffer capacity");
        return PKG_INVALID_STREAM;
    }
    if (rb_ == nullptr) {
        PKG_LOGE("rb_ is nullptr");
        UPDATER_LAST_WORD(PKG_INVALID_STREAM, "rb_ is nullptr");
        return PKG_INVALID_STREAM;
    }

    size_t needWriteLen = size;
    size_t writedLen = 0;
    while (needWriteLen > 0) {
        sem_wait(&rb_->semEmpty);
        
        size_t writingLen = needWriteLen < SINGLE_BLOCK_SIZE ? needWriteLen : SINGLE_BLOCK_SIZE;
        if (!(rb_->data[rb_->tail].Push(data.buffer + writedLen, writingLen))) {
            PKG_LOGE("push error, start : %zu, size : %zu", start, size);
            return PKG_INVALID_STREAM;
        }
        rb_->tail = (rb_->tail + 1) % BLOCK_NUM;
        needWriteLen -= writingLen;
        writedLen += writingLen;

        sem_post(&rb_->semFull);
    }

    offset_ += size;
    return PKG_SUCCESS;
}

void ShmDataStream::Stop()
{
    // 使用者取消关联共享内存
    if (rb_ == nullptr) {
        return;
    }
    if (munmap(rb_, sizeof(ShmRingBuffer)) != 0) {
        PKG_LOGE("munmap failed : %s", strerror(errno));
        return;
    }
    rb_ = nullptr;
}

void ShmDataStream::Exit()
{
    // 创建者销毁共享内存
    if (rb_ == nullptr) {
        return;
    }
    if (munmap(rb_, sizeof(ShmRingBuffer)) != 0) {
        PKG_LOGE("munmap failed : %s", strerror(errno));
        return;
    }
    rb_ = nullptr;
    if (shm_unlink(shmId_.c_str()) != 0) {
        PKG_LOGE("shm_unlink failed : %s", strerror(errno));
    }
}
#endif

MemoryMapStream::~MemoryMapStream()
{
    if (memMap_ == nullptr) {
        PKG_LOGE("Invalid memory map");
        return;
    }
    if (streamType_ == PkgStreamType_MemoryMap) {
        ReleaseMemory(memMap_, memSize_);
    }
}

int32_t MemoryMapStream::Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen)
{
    if (memMap_ == nullptr) {
        PKG_LOGE("Invalid memory map");
        return PKG_INVALID_STREAM;
    }
    if (start > memSize_) {
        PKG_LOGE("Invalid start");
        return PKG_INVALID_STREAM;
    }
    if (data.length < needRead) {
        PKG_LOGE("insufficient buffer capacity");
        return PKG_INVALID_STREAM;
    }
    size_t copyLen = GetFileLength() - start;
    readLen = ((copyLen > needRead) ? needRead : copyLen);
    if (data.data.size() == 0) {
        data.buffer = memMap_ + start;
    } else {
        if (memcpy_s(data.buffer, needRead, memMap_ + start, readLen) != EOK) {
            PKG_LOGE("Memcpy failed size:%zu, start:%zu copyLen:%zu %zu", needRead, start, copyLen, readLen);
            return PKG_NONE_MEMORY;
        }
    }
    return PKG_SUCCESS;
}

int32_t MemoryMapStream::Write(const PkgBuffer &data, size_t size, size_t start)
{
    if (memMap_ == nullptr) {
        PKG_LOGE("Invalid memory map");
        return PKG_INVALID_STREAM;
    }
    if (start > memSize_) {
        PKG_LOGE("Invalid start");
        return PKG_INVALID_STREAM;
    }

    currOffset_ = start;
    size_t copyLen = (memSize_ - start > SECUREC_MEM_MAX_LEN) ? SECUREC_MEM_MAX_LEN : memSize_ - start;
    if (copyLen < size) {
        PKG_LOGE("Write fail copyLen %zu, %zu", copyLen, size);
        return PKG_INVALID_STREAM;
    }
    int32_t ret = memcpy_s(memMap_ + currOffset_, copyLen, data.buffer, size);
    if (ret != PKG_SUCCESS) {
        PKG_LOGE("Write fail");
        return PKG_INVALID_STREAM;
    }
    PostDecodeProgress(POST_TYPE_DECODE_PKG, size, nullptr);
    return PKG_SUCCESS;
}

int32_t MemoryMapStream::Flush(size_t size)
{
    if (size != memSize_) {
        PKG_LOGE("Flush size %zu local size:%zu", size, memSize_);
    }
    if (streamType_ == PkgStreamType_MemoryMap) {
        msync(static_cast<void *>(memMap_), memSize_, MS_ASYNC);
    }
    currOffset_ = size;
    return PKG_SUCCESS;
}

int32_t MemoryMapStream::Seek(long int offset, int whence)
{
    if (whence == SEEK_SET) {
        if (offset < 0) {
            PKG_LOGE("Invalid offset");
            return PKG_INVALID_STREAM;
        }
        if (static_cast<size_t>(offset) > memSize_) {
            PKG_LOGE("Invalid offset");
            return PKG_INVALID_STREAM;
        }
        currOffset_ = static_cast<size_t>(offset);
    } else if (whence == SEEK_CUR) {
        if (static_cast<size_t>(offset) > (memSize_ - currOffset_)) {
            PKG_LOGE("Invalid offset");
            return PKG_INVALID_STREAM;
        }
        currOffset_ += static_cast<size_t>(offset);
    } else {
        if (offset > 0) {
            PKG_LOGE("Invalid offset");
            return PKG_INVALID_STREAM;
        }
        auto memSize = static_cast<long long>(memSize_);
        if (memSize + offset < 0) {
            PKG_LOGE("Invalid offset");
            return PKG_INVALID_STREAM;
        }
        currOffset_ = static_cast<size_t>(memSize + offset);
    }
    return PKG_SUCCESS;
}

int32_t ProcessorStream::Write(const PkgBuffer &data, size_t size, size_t start)
{
    if (processor_ == nullptr) {
        PKG_LOGE("processor not exist");
        return PKG_INVALID_STREAM;
    }
    int ret = processor_(data, size, start, false, context_);
    PostDecodeProgress(POST_TYPE_DECODE_PKG, size, nullptr);
    return ret;
}

int32_t ProcessorStream::Flush(size_t size)
{
    UNUSED(size);
    if (processor_ == nullptr) {
        PKG_LOGE("processor not exist");
        return PKG_INVALID_STREAM;
    }
    PkgBuffer data = {};
    return processor_(data, 0, 0, true, context_);
}

int32_t FlowDataStream::Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen)
{
    if (readOffset_ != start) {
        PKG_LOGE("Invalid start, readOffset_: %d, start: %d", readOffset_, start);
        return PKG_INVALID_STREAM;
    }

    if (data.length < needRead) {
        PKG_LOGE("Invalid need length");
        return PKG_INVALID_STREAM;
    }

    if (data.buffer == nullptr) {
        data.data.resize(needRead);
        data.buffer = data.data.data();
    }

    readLen = 0;
    uint8_t *buffer = nullptr;
    while (needRead - readLen > 0) {
        uint32_t readOnce = 0;
        if (ReadFromRingBuf(buffer, needRead - readLen, readOnce) != PKG_SUCCESS) {
            PKG_LOGE("Fail to read header");
            return PKG_INVALID_STREAM;
        }
        if (buffer == nullptr || readOnce == 0) {
            PKG_LOGE("Fail to read header, readOnce: %d", readOnce);
            return PKG_INVALID_STREAM;
        }
        if (memcpy_s(data.buffer + readLen, readOnce, buffer, readOnce) != EOK) {
            PKG_LOGE("Memcpy failed size:%zu, copyLen:%zu", needRead, readOnce);
            return PKG_NONE_MEMORY;
        }
        readLen += readOnce;
    }
    readOffset_ += needRead;
    return PKG_SUCCESS;
}

int32_t FlowDataStream::ReadFromRingBuf(uint8_t *&buff, const uint32_t needLen, uint32_t &readLen)
{
    if (ringBuf_ == nullptr) {
        PKG_LOGE("ringBuf_ is nullptr");
        buff = nullptr;
        return PKG_INVALID_STREAM;
    }

    // buf_ is empty, read from ringbuf
    if ((avail_ == 0) && !ringBuf_->Pop(buff_, MAX_FLOW_BUFFER_SIZE, avail_)) {
        PKG_LOGE("read data fail");
        buff = nullptr;
        return PKG_INVALID_STREAM;
    }

    buff = buff_ + bufOffset_;
    readLen = needLen <= avail_ ? needLen : avail_;
    avail_ -= readLen;
    bufOffset_ = avail_ == 0 ? 0 : bufOffset_ + readLen;
    return PKG_SUCCESS;
}

int32_t FlowDataStream::Write(const PkgBuffer &data, size_t size, size_t start)
{
    if (ringBuf_ == nullptr) {
        PKG_LOGE("ringBuf_ is nullptr");
        return PKG_INVALID_STREAM;
    }

    if (writeOffset_ != start) {
        PKG_LOGE("Invalid start, writeOffset: %zu, start: %zu", writeOffset_, start);
        return PKG_INVALID_STREAM;
    }

    if (ringBuf_->Push(data.buffer, size)) {
        writeOffset_ += size;
        return PKG_SUCCESS;
    }
    PKG_LOGE("Write ring buffer fail");
    return PKG_INVALID_STREAM;
}

void FlowDataStream::Stop()
{
    PKG_LOGI("FlowDataStream stop");
    if (ringBuf_ != nullptr) {
        ringBuf_->Stop();
    }
}
} // namespace Hpackage
