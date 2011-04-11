/*
 * Copyright 2011 Marshmallow Engine. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHMALLOW ENGINE ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MARSHMALLOW ENGINE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Marshmallow Engine.
 */

#include "game/engine.h"

/*!
 * @file
 *
 * @author Guillermo A. Amaral B. (gamaral) <g@maral.me>
 */

#include "core/platform.h"
#include "event/ieventmanager.h"
#include "game/iscene.h"
#include "game/scenemanager.h"
#include "game/viewmanager.h"

MARSHMALLOW_NAMESPACE_USE;
using namespace Core;
using namespace Event;
using namespace Game;

Engine *Engine::s_instance = 0;

Engine::Engine(float f, float u)
    : m_event_manager(),
      m_scene_manager(),
      m_view_manager(),
      m_fps(f),
      m_ups(u),
      m_delta_time(0),
      m_exit_code(0),
      m_frame_rate(0),
      m_running(false)
{
	if (!s_instance)
		s_instance = this;
	else
		WARNING1("Warning: Started a second engine!");
}

Engine::~Engine(void)
{
	if (this == s_instance)
		s_instance = 0;
}

void
Engine::initialize(void)
{
	Platform::Initialize();
}

void
Engine::finalize(void)
{
	m_view_manager.clear();
	m_scene_manager.clear();
	m_event_manager.clear();

	Platform::Finalize();
}

int
Engine::run(void)
{
	/*************************************************** hybrid-busy-wait */
	/*
	 * XXX: I got this idea a while back, worth try I always say 
	 */
	const TIME l_mpf = static_cast<TIME>(floor(1000/m_fps)); // milliseconds per frame
	const TIME l_mpu = static_cast<TIME>(floor(1000/m_ups)); // milliseconds per update

	TIME l_tick;
	TIME l_tick_target = MIN(l_mpu, l_mpf) / 2;

	TIME l_render = 0;
	TIME l_render_target = l_mpf;

	TIME l_update = 0;
	TIME l_update_target = l_mpu;

	TIME l_second = 0;
	TIME l_second_target = 1000;

	TIME l_lasttick = Platform::TimeStamp();

	m_delta_time = 0;
	m_running = true;

	initialize();

	/* start us off */
	TIME l_timeout = INFINITE;

	preTick(l_timeout);
	tick(l_timeout);
	postTick(l_timeout);

	preUpdate(l_timeout);
	update(l_timeout);
	postUpdate(l_timeout);

	preRender();
	render();
	postRender();

	/* main game loop */
	do
	{
		l_timeout = l_tick_target - 1;

		l_tick = Platform::TimeStamp();
		m_delta_time = l_tick - l_lasttick;

		l_render += m_delta_time;
		l_update += m_delta_time;
		l_second += m_delta_time;

		/* tick */
		preTick(l_timeout);
		tick(l_timeout);
		postTick(l_timeout);

		if (l_render >= l_render_target) {
			/* render */
			preRender();
			render();
			postRender();

			l_render += (Platform::TimeStamp() - l_tick) - l_render_target;
			++m_frame_rate;
		}

		if (l_update >= l_update_target) {
			/* update */
			preUpdate(l_timeout);
			update(l_timeout);
			postUpdate(l_timeout);

			l_update += (Platform::TimeStamp() - l_tick) - l_update_target;
		}

		if (l_second >= l_second_target) {
			if (m_frame_rate < m_fps && l_tick_target > 1) {
				l_tick_target -= 1;
				INFO("Framerate dropping! adjusting tick target (%d).",
				    static_cast<int>(l_tick_target));
			} else if (m_frame_rate > m_fps * 1.10) {
				l_tick_target += 2;
				INFO("Framerate accelerated! adjusting tick target (%d).",
				    static_cast<int>(l_tick_target));
			}

			/* second */
			preSecond();
			second();
			postSecond();

			l_second += (Platform::TimeStamp() - l_tick) - l_second_target;
		}

		l_lasttick = l_tick;

		Platform::Sleep(l_tick_target);
	} while (m_running);

	finalize();

	return(m_exit_code);
}

SharedEventManager
Engine::eventManager(void) const
{
	return(m_event_manager);
}

SharedSceneManager
Engine::sceneManager(void) const
{
	return(m_scene_manager);
}

SharedViewManager
Engine::viewManager(void) const
{
	return(m_view_manager);
}

void
Engine::setEventManager(SharedEventManager &m)
{
	m_event_manager = m;
}

void
Engine::setSceneManager(SharedSceneManager &m)
{
	m_scene_manager = m;
}

void
Engine::setViewManager(SharedViewManager &m)
{
	m_view_manager = m;
}

void
Engine::render(void)
{
	SharedScene l_scene = m_scene_manager->active();
	if (m_view_manager) {
		m_view_manager->render(l_scene);
	} else WARNING1("No view manager!");
}

void
Engine::tick(TIME &timeout)
{
	TIMEOUT_INIT;
	Platform::PreTick(TIMEOUT_DEC(timeout));

	if (m_event_manager) {
		m_event_manager->execute(TIMEOUT_DEC(timeout));
	} else WARNING1("No event manager!");
}

void
Engine::update(TIME &timeout)
{
	TIMEOUT_INIT;
	Platform::PreUpdate(TIMEOUT_DEC(timeout));

	if (m_scene_manager) {
		SharedScene l_scene = m_scene_manager->active();
		if (l_scene) l_scene->update();
		else WARNING1("No active scene!");
	} else WARNING1("No scene manager!");
}

void
Engine::preRender(void)
{
	Platform::PreRender();
}

void
Engine::postRender(void)
{
	Platform::PostRender();
}

void
Engine::preSecond(void)
{
	Platform::PreSecond();
}

void
Engine::postSecond(void)
{
	INFO("FPS %d!", m_frame_rate);
	m_frame_rate = 0;

	Platform::PostSecond();
}

void
Engine::preTick(TIME &timeout)
{
	UNUSED(timeout);
}

void
Engine::postTick(TIME &timeout)
{
	TIMEOUT_INIT;
	Platform::PostTick(TIMEOUT_DEC(timeout));
}

void
Engine::preUpdate(TIME &timeout)
{
	UNUSED(timeout);
}

void
Engine::postUpdate(TIME &timeout)
{
	TIMEOUT_INIT;
	Platform::PostUpdate(TIMEOUT_DEC(timeout));
}

