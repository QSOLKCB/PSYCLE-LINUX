// This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
// copyright 2003-2007 johan boule <bohan@jabber.org>
// copyright 2003-2007 psycledelics http://psycle.pastnotecut.org

/// \file
/// \brief delay
#include "../detail/prefix.h"


#include "plugin.hpp"
#include <psycle/helpers/math/erase_all_nans_infinities_and_denormals.hpp>
#include <cassert>
#include <vector>
namespace psycle { namespace plugin {
	using namespace psycle::helpers::math;


class Delay : public Plugin
{
public:
	/*override*/ void help(std::ostream & out) throw()
	{
		out << "Delay." << std::endl;
		out << "Compatible with original psycle 1 arguru's dala delay." << std::endl;
		out << std::endl;
		out << "Beware if you tweak the delay length with a factor > 2 that the memory buffer gets resized." << std::endl;
	}
	enum Parameters
	{
		dry, wet,
		left_delay, left_feedback,
		right_delay, right_feedback,
		snap
	};

	static const Information & information() throw()
	{
		static bool initialized = false;
		static Information *info = NULL;
		if (!initialized) {
			const int factors(2 * 3 * 4 * 5 * 7);
			const Real delay_maximum(Information::Parameter::input_maximum_value / factors);
			static const Information::Parameter parameters [] =
			{
				Information::Parameter::linear("dry", -1, 1, 1),
				Information::Parameter::linear("wet", -1, 0, 1),
				Information::Parameter::linear("delay left", 0, 0, delay_maximum),
				Information::Parameter::linear("feedback left", -1, 0, 1),
				Information::Parameter::linear("delay right", 0, 0, delay_maximum),
				Information::Parameter::linear("feedback right", -1, 0, 1),
				Information::Parameter::discrete("snap to", 3, factors - 1)
			};
			static Information information(0x0110, Information::Types::effect, "ayeternal Dalay Delay", "Dalay Delay", "bohan", 4, parameters, sizeof parameters / sizeof *parameters);
			info = &information;
			initialized = true;
		}
		return *info;
	}

	/*override*/ void describe(std::ostream & out, const int & parameter) const
	{
		switch(parameter)
		{
		case left_delay:
		case right_delay:
			out << (*this)(parameter) << " ticks (lines)";
			break;
		case snap:
			if((*this)[parameter] == information().parameter(parameter).MaxValue) out << "off ";
			out << "1 / " << 1 + (*this)[parameter] << " ticks (lines)";
			break;
		case left_feedback:
		case right_feedback:
		case dry:
		case wet:
			if(std::fabs((*this)(parameter)) < 2e-5)
			{
				out << 0;
				break;
			}
		default:
			Plugin::describe(out, parameter);
		}
	}

	Delay() : Plugin(information())
	{
		requested_delay_values_[left] = information().parameter(left_delay).DefValue;
		requested_delay_values_[right] = information().parameter(right_delay).DefValue;
	}
	/*override*/ void init();
	/*override*/ void Work(Sample l [], Sample r [], int samples, int);
	/*override*/ void parameter(const int &);
protected:
	/*override*/ void samples_per_second_changed()
	{
		resize(left, (*this)(left_delay));
		resize(right, (*this)(right_delay));
	}
	/*override*/ void sequencer_ticks_per_second_changed()
	{
		resize(left, (*this)(left_delay));
		resize(right, (*this)(right_delay));
	}
	enum Channels { left, right, channels };
	std::vector<Real> buffers_ [channels];
	std::vector<Real>::iterator buffer_iterators_ [channels];
	int requested_delay_values_ [channels];
	inline void Work(std::vector<Real> & buffer, std::vector<Real>::iterator & buffer_iterator, Sample & input, const Sample & feedback);
	inline void apply_requested_delay(const int & channel, const int & parameter,
		const int & requested_value);
	inline void resize(const int & channel, const Real & delay);
};

PSYCLE__PLUGIN__INSTANTIATOR(Delay)

void Delay::init()
{
	for(int channel(0) ; channel < channels ; ++channel)
		resize(channel, Real(0)); // resizes the buffers not to 0, but to 1, the smallest length possible for the algorithm to work
}

void Delay::parameter(const int & parameter)
{
	switch(parameter)
	{
	case left_delay:
		requested_delay_values_[left] = (*this)[left_delay];
		apply_requested_delay(left, left_delay, requested_delay_values_[left]);
		break;
	case right_delay:
		requested_delay_values_[right] = (*this)[right_delay];
		apply_requested_delay(right, right_delay, requested_delay_values_[right]);
		break;
	case snap:
		/* Presets and PSY3 restore public parameters in index order, so a delay
		** can arrive while the default snap grid is still active.  If that
		** earlier quantisation changed the requested raw value, reapply the
		** retained request now that the saved snap value has arrived.  Exact
		** on-grid delays remain untouched, preserving historical behavior. */
		if ((*this)[left_delay] != requested_delay_values_[left])
			apply_requested_delay(left, left_delay, requested_delay_values_[left]);
		if ((*this)[right_delay] != requested_delay_values_[right])
			apply_requested_delay(right, right_delay, requested_delay_values_[right]);
		break;
	}
}

inline void Delay::apply_requested_delay(const int & channel,
	const int & parameter, const int & requested_value)
{
	const int snap1((*this)[snap] + 1);
	const Real requested_delay(information().parameter(parameter).scale.apply(
		static_cast<Real>(requested_value)));
	const Real snap_delay(static_cast<int>(requested_delay * snap1) /
		static_cast<Real>(snap1));
	(*this)(parameter) = snap_delay;
	(*this)[parameter] = information().parameter(parameter).scale.apply_inverse(snap_delay) + 1; // Round up.
	resize(channel, snap_delay);
}

inline void Delay::resize(const int & channel, const Real & delay)
{
	buffers_[channel].resize(1 + static_cast<int>(delay * samples_per_sequencer_tick()), 0);
			// resizes the buffer at least to 1, the smallest length possible for the algorithm to work
	buffer_iterators_[channel] = buffers_[channel].begin();
}

void Delay::Work(Sample l [], Sample r [], int samples, int)
{
	for(int sample(0) ; sample < samples ; ++sample)
	{
		Work(buffers_[left] , buffer_iterators_[left] , l[sample], (*this)(left_feedback));
		Work(buffers_[right], buffer_iterators_[right], r[sample], (*this)(right_feedback));
	}
}

inline void Delay::Work(std::vector<Real> & buffer, std::vector<Real>::iterator & buffer_iterator, Sample & input, const Sample & feedback)
{
	const Real read(*buffer_iterator);
	Real newval = input + feedback * read;
	erase_all_nans_infinities_and_denormals(newval);
	*buffer_iterator = newval;
	if (++buffer_iterator == buffer.end()) buffer_iterator = buffer.begin();
	input = static_cast<Sample>((*this)(dry) * input + (*this)(wet) * read);
}

}}
