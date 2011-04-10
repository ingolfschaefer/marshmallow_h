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

#include "event/debugeventlistener.h"
#include "event/eventbase.h"
#include "event/eventmanager.h"
#include "game/engine.h"
#include "game/scenemanager.h"
#include "game/scenebase.h"

MARSHMALLOW_NAMESPACE_USE;
using namespace Core;

class Demo : public Game::Engine
{
	int m_stop_timer;
	Event::SharedEventManager  m_event_manager;
	Game::SharedSceneManager   m_scene_manager;
	Event::SharedEventListener m_debugListener;

public:
	Demo(void)
	: Engine(),
	  m_stop_timer(0),
	  m_event_manager(new Event::EventManager("main")),
	  m_scene_manager(new Game::SceneManager(0)),
	  m_debugListener(new Event::DebugEventListener("log.txt"))
	{
	}

	VIRTUAL void initialize(void)
	{
		Engine::initialize();

		setEventManager(m_event_manager);
		setSceneManager(m_scene_manager);

		eventManager()->connect(m_debugListener, Event::EventBase::Type);
	}

	VIRTUAL void finalize(void)
	{
		eventManager()->disconnect(m_debugListener, Event::EventBase::Type);
		
		Engine::finalize();
	}

	VIRTUAL void second(void)
	{
		Event::SharedEvent event(new Event::EventBase("event"));
		Event::SharedEvent event2(new Event::EventBase("event2"));

		eventManager()->queue(event);
		eventManager()->queue(event2);

		/*
		 * Dequeue all on odd seconds
		 */
		eventManager()->dequeue(event, m_stop_timer % 2);

		if (++m_stop_timer == 10)
			stop();
	}
};

int
main(void)
{
	return(Demo().run());
}
