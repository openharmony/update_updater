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
#ifndef PKG_STREAM_H
#define PKG_STREAM_H
#ifndef __WIN32
#include <sys/mman.h>
#endif
#include <atomic>
#include <mutex>
#include <semaphore.h>
#include "pkg_manager.h"
#include "pkg_utils.h"

namespace Updater {
class RingBuffer;
}

namespace Hpackage {
class PkgStreamImpl : public PkgStream {
public:
    explicit PkgStreamImpl(PkgManager::PkgManagerPtr pkgManager, const std::string fileName)
        : fileName_(fileName), refCount_(0), pkgManager_(pkgManager) {}

    virtual ~PkgStreamImpl() {}

    int32_t Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen) override
    {
        UNUSED(data);
        UNUSED(start);
        UNUSED(readLen);
        UNUSED(needRead);
        return PKG_SUCCESS;
    }

    int32_t GetBuffer(PkgBuffer &buffer) const override
    {
        buffer.length = 0;
        buffer.buffer = nullptr;
        return PKG_SUCCESS;
    }

    int32_t Write(const PkgBuffer &data, size_t size, size_t start) override
    {
        UNUSED(data);
        UNUSED(size);
        UNUSED(start);
        return PKG_SUCCESS;
    }

    virtual int32_t Seek(long int offset, int whence) = 0;

    int32_t Flush(size_t size) override
    {
        UNUSED(size);
        return PKG_SUCCESS;
    }

    const std::string GetFileName() const override;

    int32_t GetStreamType() const override
    {
        return PkgStreamType_Read;
    };

    void AddRef() override;

    void DelRef() override;

    bool IsRef() const override;

    static PkgStreamPtr ConvertPkgStream(PkgManager::StreamPtr stream);

protected:
    void PostDecodeProgress(int type, size_t writeDataLen, const void *context) const;
    std::string fileName_;

private:
    std::atomic_int refCount_;
    PkgManager::PkgManagerPtr pkgManager_ = nullptr;
};

class FileStream : public PkgStreamImpl {
public:
    FileStream(PkgManager::PkgManagerPtr pkgManager, const std::string fileName, FILE *stream, int32_t streamType)
        : PkgStreamImpl(pkgManager, fileName), stream_(stream), fileLength_(0), streamType_(streamType) {}

    ~FileStream() override;

    int32_t Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen) override;

    int32_t Write(const PkgBuffer &data, size_t size, size_t start) override;

    int32_t Seek(long int offset, int whence) override;

    int32_t Flush(size_t size) override;

    size_t GetFileLength() override;

    int32_t GetStreamType() const override
    {
        return streamType_;
    }

private:
    FILE *stream_;
    size_t fileLength_;
    int32_t streamType_;
    std::recursive_mutex fileStreamLock_;
};

constexpr int32_t BLOCK_NUM =32;
constexpr size_t SINGLE_BLOCK_SIZE = 50 * 1024;
constexpr size_t RING_BUFFER_SIZE = BLOCK_NUM * SINGLE_BLOCK_SIZE;

class ShmRbBlock {
public:
    bool Push(const uint8_t* buf, size_t len);
    bool Pop(uint8_t* buf, size_t expectedLen, size_t &realLen);
    size_t GetRealLen();

private:
    size_t realLen_ = 0;              // 块内偏移数据
    size_t blkOffset_ = 0;            // 头偏移
    uint8_t data_[SINGLE_BLOCK_SIZE]; // 数据域
};

struct ShmRingBuffer {
    sem_t sem_empty;            // 空闲块信号量
    sem_t sem_full;             // 已用块信号量
    int32_t head = 0;           // 消费者写入位置
    int32_t tail = 0;           // 生产者写入位置
    ShmRbBlock data[BLOCK_NUM]; // 环形数据缓冲区
}

struct ShmParameter {
    std::string shmId;
    size_t pkgLen = 0;
    size_t offset = 0;

    ShmParameter(const std::string &shmId, size_t pkgLen, size_t offset)
        : shmId(shmId), pkgLen(pkgLen), offset(offset) {}
}

class ShmDataStream : public PkgStreamImpl {
public:
    ShmDataStream(PkgManager::PkgManagerPtr pkgManager, const std::string fileName, const ShmParameter &ShmParameter, 
        int32_t streamType) : PkgStreamImpl(pkgManager, fileName), streamType_(streamType) 
    {
        shmId_ = ShmParameter.shmId;
        pkgLen_ = ShmParameter.pkgLen;
        offset_ = ShmParameter.offset;
    }

    ~ShmDataStream() override
    {
        Stop();
    }

    int32_t CreateShmRingBuffer();
    
    int32_t InitShmRingBuffer();
    
    int32_t Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen) override;

    int32_t Write(const PkgBuffer &data, size_t size, size_t start);

    void Stop() override;

    void Exit();

    int32_t Seek(long int size, int whence) override
    {
        UNUSED(size);
        UNUSED(whence);
        return PKG_SUCCESS;
    }

    void SetOffset(size_t offset)
    {
        offset_ = offset;
    }

    size_t GetReadOffset() const override
    {
        return offset_;
    }

    void SetPkgLen(size_t pkgLen)
    {
        pkgLen_ = pkgLen;
    }

    size_t GetFileLength() override
    {
        return pkgLen_;
    }

    int32_t GetStreamType() const override
    {
        return streamType_;
    }

private:
    ShmRingBuffer* rb_ = nullptr;
    std::string shmId_;
    int32_t streamType_ = PkgStreamType_ShmData;
    size_t pkgLen_ = 0; // 整包大小
    size_t offset_ = 0; // 偏移
};

class MemoryMapStream : public PkgStreamImpl {
public:
    MemoryMapStream(PkgManager::PkgManagerPtr pkgManager, const std::string fileName, const PkgBuffer &buffer,
        int32_t streamType = PkgStreamType_MemoryMap) : PkgStreamImpl(pkgManager, fileName), memMap_(buffer.buffer),
        memSize_(buffer.length), currOffset_(0), streamType_(streamType) {}
    ~MemoryMapStream() override;

    int32_t Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen) override;

    int32_t Write(const PkgBuffer &data, size_t size, size_t start) override;

    int32_t Seek(long int offset, int whence) override;

    int32_t GetStreamType() const override
    {
        return streamType_;
    }

    size_t GetFileLength() override
    {
        return memSize_;
    }

    int32_t Flush(size_t size) override
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

    int32_t GetBuffer(PkgBuffer &buffer) const override
    {
        buffer.buffer = memMap_;
        buffer.length = memSize_;
        return PKG_SUCCESS;
    }

private:
    uint8_t *memMap_;
    size_t memSize_;
    size_t currOffset_;
    int32_t streamType_;
};

class ProcessorStream : public PkgStreamImpl {
public:
    ProcessorStream(PkgManager::PkgManagerPtr pkgManager, const std::string fileName,
        ExtractFileProcessor processor, const void *context)
        : PkgStreamImpl(pkgManager, fileName), processor_(processor), context_(context) {}

    ~ProcessorStream() override {}

    int32_t Read(PkgBuffer &data, size_t start, size_t needRead, size_t &readLen) override
    {
        UNUSED(data);
        UNUSED(start);
        UNUSED(readLen);
        UNUSED(needRead);
        return PKG_INVALID_STREAM;
    }

    int32_t Write(const PkgBuffer &data, size_t size, size_t start) override
    {
        if (processor_ == nullptr) {
            PKG_LOGE("processor not exist");
            return PKG_INVALID_STREAM;
        }
        int ret = processor_(data, size, start, false, context_);
        PostDecodeProgress(POST_TYPE_DECODE_PKG, size, nullptr);
        return ret;
    }

    int32_t Seek(long int size, int whence) override
    {
        UNUSED(size);
        UNUSED(whence);
        return PKG_SUCCESS;
    }

    int32_t GetStreamType() const override
    {
        return PkgStreamType_Process;
    }

    size_t GetFileLength() override
    {
        return 0;
    }

    int32_t Flush(size_t size) override
    {
        UNUSED(size);
        if (processor_ == nullptr) {
            PKG_LOGE("processor not exist");
            return PKG_INVALID_STREAM;
        }
        PkgBuffer data = {};
        return processor_(data, 0, 0, true, context_);
    }

private:
    ExtractFileProcessor processor_ = nullptr;
    const void *context_;
};

constexpr uint32_t MAX_FLOW_BUFFER_SIZE = 4 * 1024 * 1024;

class FlowDataStream : public Hpackage::PkgStreamImpl {
public:
    FlowDataStream(Hpackage::PkgManager::PkgManagerPtr pkgManager, const std::string fileName,
        const size_t fileSize, Updater::RingBuffer *buffer, int32_t streamType =
        PkgStreamType_FlowData) : PkgStreamImpl(pkgManager, fileName), fileLength_(fileSize),
        ringBuf_(buffer), streamType_(streamType) {}
    ~FlowDataStream() override {}

    int32_t Read(Hpackage::PkgBuffer &data, size_t start, size_t needRead, size_t &readLen) override;

    int32_t Write(const Hpackage::PkgBuffer &data, size_t size, size_t start) override;

    int32_t Seek(long int offset, int whence) override
    {
        UNUSED(offset);
        UNUSED(whence);
        return Hpackage::PKG_INVALID_STREAM;
    }

    int32_t GetStreamType() const override
    {
        return streamType_;
    }

    size_t GetFileLength() override
    {
        return fileLength_;
    }

    int32_t Flush(size_t size) override
    {
        UNUSED(size);
        return Hpackage::PKG_INVALID_STREAM;
    }

    size_t GetReadOffset() const override
    {
        return readOffset_;
    }

    void Stop() override;

private:
    int32_t ReadFromRingBuf(uint8_t *&buff, const uint32_t needLen, uint32_t &readLen);

    size_t fileLength_ {};
    Updater::RingBuffer *ringBuf_ {};
    int32_t streamType_;
    uint8_t buff_[MAX_FLOW_BUFFER_SIZE] = {0};
    uint32_t avail_ {};
    uint32_t bufOffset_ {};
    size_t readOffset_ {};
    size_t writeOffset_ {};
};
} // namespace Hpackage
#endif // PKG_STREAM_H
