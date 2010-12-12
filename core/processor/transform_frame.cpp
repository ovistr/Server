#include "../StdAfx.h"

#include "transform_frame.h"

#include "draw_frame.h"
#include "frame_shader.h"

#include "../format/pixel_format.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/pixel_buffer_object.h"

#include <boost/range/algorithm.hpp>

#include <tbb/parallel_for.h>

namespace caspar { namespace core {
																																						
struct transform_frame::implementation
{
	implementation(const draw_frame& frame) : frame_(frame), audio_volume_(255), override_audio_(false){}
	implementation(const draw_frame& frame, const std::vector<short>& audio_data) : frame_(frame), audio_volume_(255), audio_data_(audio_data), override_audio_(true){}
	implementation(draw_frame&& frame) : frame_(std::move(frame)), audio_volume_(255), override_audio_(false){}
	
	void begin_write(){detail::draw_frame_access::begin_write(frame_);}
	void end_write(){detail::draw_frame_access::end_write(frame_);}

	void draw(frame_shader& shader)
	{
		shader.begin(transform_);
		detail::draw_frame_access::draw(frame_, shader);
		shader.end();
	}

	void audio_volume(unsigned char volume)
	{
		if(volume == audio_volume_)
			return;
		
		audio_volume_ = volume;
		if(!override_audio_)
			audio_data_ = frame_.audio_data();
		tbb::parallel_for
		(
			tbb::blocked_range<size_t>(0, audio_data_.size()),
			[&](const tbb::blocked_range<size_t>& r)
			{
				for(size_t n = r.begin(); n < r.end(); ++n)					
					audio_data_[n] = static_cast<short>((static_cast<int>(audio_data_[n])*audio_volume_)>>8);						
			}
		);
	}
		
	bool override_audio_;
	std::vector<short> audio_data_;
	shader_transform transform_;	
	unsigned char audio_volume_;
	draw_frame frame_;
};
	
transform_frame::transform_frame(const draw_frame& frame) : impl_(new implementation(frame)){}
transform_frame::transform_frame(const draw_frame& frame, const std::vector<short>& audio_data) : impl_(new implementation(frame, audio_data)){}
transform_frame::transform_frame(draw_frame&& frame) : impl_(new implementation(std::move(frame))){}
transform_frame::transform_frame(const transform_frame& other) : impl_(new implementation(*other.impl_)){}
transform_frame& transform_frame::operator=(const transform_frame& other)
{
	transform_frame temp(other);
	temp.impl_.swap(impl_);
	return *this;
}
transform_frame::transform_frame(transform_frame&& other) : impl_(std::move(other.impl_)){}
transform_frame& transform_frame::operator=(transform_frame&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void transform_frame::begin_write(){impl_->begin_write();}
void transform_frame::end_write(){impl_->end_write();}	
void transform_frame::draw(frame_shader& shader){impl_->draw(shader);}
void transform_frame::audio_volume(unsigned char volume){impl_->audio_volume(volume);}
void transform_frame::translate(double x, double y){impl_->transform_.pos = boost::make_tuple(x, y);}
void transform_frame::texcoord(double left, double top, double right, double bottom){impl_->transform_.uv = boost::make_tuple(left, top, right, bottom);}
void transform_frame::video_mode(video_mode::type mode){impl_->transform_.mode = mode;}
void transform_frame::alpha(double value){impl_->transform_.alpha = value;}
const std::vector<short>& transform_frame::audio_data() const { return impl_->audio_data_; }
}}