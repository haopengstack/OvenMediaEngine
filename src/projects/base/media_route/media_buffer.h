//==============================================================================
//
//  MediaRouteApplication
//
//  Created by Kwon Keuk Han
//  Copyright (c) 2018 AirenSoft. All rights reserved.
//
//==============================================================================

// 인코딩 패킷, 비디오/오디오 프레임을 저장하는 공통 버퍼

#pragma once

#include <cstdint>
#include <vector>
#include <map>

#include "media_type.h"

enum class MediaPacketFlag
{
	NoFlag,
	Key
};

class MediaPacket
{
public:
	MediaPacket(MediaCommonType::MediaType media_type, int32_t track_id, const void *data, int32_t data_size, int64_t pts, MediaPacketFlag flags)
		: _media_type(media_type),
		  _track_id(track_id),
		  _pts(pts),
		  _flags(flags)
	{
		_data->Append(data, data_size);
	}

	MediaPacket(MediaCommonType::MediaType media_type, int32_t track_id, const std::shared_ptr<ov::Data> &data, int64_t pts, MediaPacketFlag flags)
		: _media_type(media_type),
		  _track_id(track_id),
		  _pts(pts),
		  _flags(flags)
	{
		_data->Append(data);
	}

	MediaCommonType::MediaType GetMediaType() const noexcept
	{
		return _media_type;
	}

	const std::shared_ptr<const ov::Data> GetData() const noexcept
	{
		return _data;
	}

	std::shared_ptr<ov::Data> &GetData() noexcept
	{
		return _data;
	}

	int64_t GetPts() const noexcept
	{
		return _pts;
	}

	int32_t GetTrackId() const noexcept
	{
		return _track_id;
	}

	void SetTrackId(int32_t track_id)
	{
		_track_id = track_id;
	}

	MediaPacketFlag GetFlags() const noexcept
	{
		return _flags;
	}

protected:
	MediaCommonType::MediaType _media_type;
	int32_t _track_id;

	std::shared_ptr<ov::Data> _data = ov::Data::CreateData();

	int64_t _pts;
	MediaPacketFlag _flags;
};

class MediaFrame
{
public:
	MediaFrame() = default;

	MediaFrame(MediaCommonType::MediaType media_type, int32_t track_id, const uint8_t *data, int32_t data_size, int64_t pts, int32_t flags)
		: _media_type(media_type),
		  _track_id(track_id),

		  _flags(flags),

		  _pts(pts)
	{
		SetBuffer(data, data_size);
	}

	MediaFrame(MediaCommonType::MediaType media_type, int32_t track_id, const uint8_t *data, int32_t data_size, int64_t pts)
		: MediaFrame(media_type, track_id, data, data_size, pts, 0)
	{
	}

	MediaFrame(const uint8_t *data, int32_t data_size, int64_t pts)
		: MediaFrame(MediaCommonType::MediaType::Unknown, 0, data, data_size, pts, 0)
	{
	}

	~MediaFrame() = default;

	void ClearBuffer(int32_t plane = 0)
	{
		_data_buffer[plane].clear();
	}

	void SetBuffer(const uint8_t *data, int32_t data_size, int32_t plane = 0)
	{
		_data_buffer[plane].clear();
		_data_buffer[plane].insert(_data_buffer[plane].end(), data, data + data_size);
	}

	void AppendBuffer(const uint8_t *data, int32_t data_size, int32_t plane = 0)
	{
		_data_buffer[plane].insert(_data_buffer[plane].end(), data, data + data_size);
	}

	void AppendBuffer(uint8_t byte, int32_t plane = 0)
	{
		_data_buffer[plane].push_back(byte);
	}

	void InsertBuffer(int offset, const uint8_t *data, int32_t data_size, int32_t plane = 0)
	{
		_data_buffer[plane].insert(_data_buffer[plane].begin() + offset, data, data + data_size);
	}

	const uint8_t *GetBuffer(int32_t plane = 0) const
	{
		auto list = GetPlainData(plane);

		if(list != nullptr)
		{
			return list->data();
		}

		return nullptr;
	}

	uint8_t *GetBuffer(int32_t plane = 0)
	{
		return reinterpret_cast<uint8_t *>(_data_buffer[plane].data());
	}

	uint8_t GetByteAt(int32_t offset, int32_t plane = 0) const
	{
		auto list = GetPlainData(plane);

		if(list != nullptr)
		{
			return (*list)[offset];
		}

		return 0;
	}

	size_t GetDataSize(int32_t plane = 0) const
	{
		auto list = GetPlainData(plane);

		if(list != nullptr)
		{
			return list->size();
		}

		return 0;
	}

	size_t GetBufferSize(int32_t plane = 0) const
	{
		auto list = GetPlainData(plane);

		if(list != nullptr)
		{
			return list->size();
		}

		return 0;
	}

	void EraseBuffer(int32_t offset, int32_t length, int32_t plane = 0)
	{
		_data_buffer[plane].erase(_data_buffer[plane].begin() + offset, _data_buffer[plane].begin() + offset + length);
	}

	// 메모리만 미리 할당함
	void Reserve(uint32_t capacity, int32_t plane = 0)
	{
		_data_buffer[plane].reserve(static_cast<unsigned long>(capacity));
	}

	// 메모리 할당 및 데이터 오브젝트 할당
	// Append Buffer의 성능문제로 Resize를 선작업한다음 GetBuffer로 포인터를 얻어와 데이터를 설정함.
	void Resize(uint32_t capacity, int32_t plane = 0)
	{
		_data_buffer[plane].resize(static_cast<unsigned long>(capacity));
	}

	void SetMediaType(MediaCommonType::MediaType media_type)
	{
		_media_type = media_type;
	}

	MediaCommonType::MediaType GetMediaType() const
	{
		return _media_type;
	}

	void SetTrackId(int32_t track_id)
	{
		_track_id = track_id;
	}

	int32_t GetTrackId() const
	{
		return _track_id;
	}

	int64_t GetPts() const
	{
		return _pts;
	}

	void SetPts(int64_t pts)
	{
		_pts = pts;
	}

	void SetOffset(size_t offset)
	{
		_offset = offset;
	}

	size_t GetOffset() const
	{
		return _offset;
	}

	void IncreaseOffset(size_t delta)
	{
		_offset += delta;

		OV_ASSERT2(_offset >= 0L);
	}

	void SetStride(int32_t stride, int32_t plane = 0)
	{
		_stride[plane] = stride;
	}

	int32_t GetStride(int32_t plane = 0) const
	{
		const auto &stride = _stride.find(plane);

		if(stride == _stride.cend())
		{
			return 0;
		}

		return stride->second;
	}

	void SetWidth(int32_t width)
	{
		_width = width;
	}

	int32_t GetWidth() const
	{
		return _width;
	}

	void SetHeight(int32_t height)
	{
		_height = height;
	}

	int32_t GetHeight() const
	{
		return _height;
	}

	void SetFormat(int32_t format)
	{
		_format = format;
	}

	int32_t GetFormat() const
	{
		return _format;
	}

	int32_t GetBytesPerSample() const
	{
		return _bytes_per_sample;
	}

	void SetBytesPerSample(int32_t bytes_per_sample)
	{
		_bytes_per_sample = bytes_per_sample;
	}

	int32_t GetNbSamples() const
	{
		return _nb_samples;
	}

	void SetNbSamples(int32_t nb_samples)
	{
		_nb_samples = nb_samples;
	}

	int32_t GetChannels() const
	{
		return _channels;
	}

	void SetChannels(int32_t channels)
	{
		_channels = channels;
	}

	MediaCommonType::AudioChannel::Layout GetChannelLayout() const
	{
		return _channel_layout;
	}

	void SetChannelLayout(MediaCommonType::AudioChannel::Layout channel_layout)
	{
		_channel_layout = channel_layout;
	}

	int32_t GetSampleRate() const
	{
		return _sample_rate;
	}

	void SetSampleRate(int32_t sample_rate)
	{
		_sample_rate = sample_rate;
	}

	void SetFlags(int32_t flags)
	{
		_flags = flags;
	}

	int32_t GetFlags()
	{
		return _flags;
	}

private:
	const std::vector<uint8_t> *GetPlainData(int32_t plane) const
	{
		auto item = _data_buffer.find(plane);

		if(item == _data_buffer.cend())
		{
			return nullptr;
		}

		return &(item->second);
	}

	// Data plane, Data
	std::map<int32_t, std::vector<uint8_t>> _data_buffer;

	// 미디어 타입
	MediaCommonType::MediaType _media_type = MediaCommonType::MediaType::Unknown;

	// 트랙 아이디
	int32_t _track_id = 0;

	// 공통 시간 정보
	int64_t _pts = 0LL;
	size_t _offset = 0;

	std::map<int32_t, int32_t> _stride;

	// 디코딩된 비디오 프레임 정보
	int32_t _width = 0;
	int32_t _height = 0;
	int32_t _format = 0;

	// 디코딩된 오디오 프레임 정보
	int32_t _bytes_per_sample = 0;
	int32_t _nb_samples = 0;
	int32_t _channels = 0;
	MediaCommonType::AudioChannel::Layout _channel_layout = MediaCommonType::AudioChannel::Layout::LayoutMono;
	int32_t _sample_rate = 0;

	int32_t _flags = 0;    // Key, non-Key
};