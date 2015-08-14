/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  The context handling the gameplay in Neverwinter Nights.
 */

#include <cassert>

#include "src/common/filepath.h"
#include "src/common/filelist.h"
#include "src/common/configman.h"

#include "src/aurora/resman.h"

#include "src/events/events.h"

#include "src/sound/sound.h"

#include "src/engines/aurora/util.h"

#include "src/engines/nwn/game.h"
#include "src/engines/nwn/nwn.h"
#include "src/engines/nwn/version.h"
#include "src/engines/nwn/console.h"
#include "src/engines/nwn/module.h"
#include "src/engines/nwn/area.h"

#include "src/engines/nwn/gui/legal.h"
#include "src/engines/nwn/gui/main/main.h"

namespace Engines {

namespace NWN {

Game::Game(NWNEngine &engine, ::Engines::Console &console, const Version &version) :
	_engine(&engine), _module(0), _console(&console), _version(&version) {

}

Game::~Game() {
	delete _module;
}

const Version &Game::getVersion() const {
	return *_version;
}

Module &Game::getModule() {
	assert(_module);

	return *_module;
}

void Game::run() {
	bool first = true;

	_module = new Module(*_console, *_version);

	while (!EventMan.quitRequested()) {
		mainMenu(first, first);

		if (EventMan.quitRequested())
			break;

		_module->run();
		_module->clear();

		first = false;
	}

	delete _module;
	_module = 0;
}

void Game::playMenuMusic(Common::UString music) {
	stopMenuMusic();

	if (music.empty())
		music = ConfigMan.getBool("NWN_hasXP2") ? "mus_x2theme" : "mus_theme_main";

	_menuMusic = playSound(music, Sound::kSoundTypeMusic, true);
}

void Game::stopMenuMusic() {
	SoundMan.stopChannel(_menuMusic);
}

void Game::playMusic(const Common::UString &music) {
	if (_module && _module->isRunning()) {
		Area *area = _module->getCurrentArea();
		if (area)
			area->playAmbientMusic(music);

		return;
	}

	playMenuMusic(music);
}

void Game::stopMusic() {
	stopMenuMusic();

	if (_module && _module->isRunning()) {
		Area *area = _module->getCurrentArea();
		if (area)
			area->stopAmbientMusic();
	}
}

void Game::mainMenu(bool playStartSound, bool showLegal) {
	playMenuMusic();

	if (playStartSound)
		playSound("gui_prompt", Sound::kSoundTypeSFX);

	EventMan.flushEvents();

	MainMenu mainMenu(*_module, _console);

	if (showLegal) {
		// Fade in, show and fade out the legal billboard

		Legal legal;

		legal.fadeIn();
		mainMenu.show();
		legal.show();
	} else
		mainMenu.show();

	_console->disableCommand("loadcampaign", "not available in the main menu");
	_console->disableCommand("loadmodule"  , "not available in the main menu");
	_console->disableCommand("exitmodule"  , "not available in the main menu");
	_console->disableCommand("listareas"   , "not available in the main menu");
	_console->disableCommand("gotoarea"    , "not available in the main menu");

	mainMenu.run();

	_console->enableCommand("loadcampaign");
	_console->enableCommand("loadmodule");
	_console->enableCommand("exitmodule");
	_console->enableCommand("listareas");
	_console->enableCommand("gotoarea");

	mainMenu.hide();

	stopMenuMusic();
}

void Game::getCharacters(std::vector<Common::UString> &characters, bool local) {
	characters.clear();

	Common::UString pcDir = ConfigMan.getString(local ? "NWN_localPCDir" : "NWN_serverPCDir");
	if (pcDir.empty())
		return;

	Common::FileList chars;
	chars.addDirectory(pcDir);

	for (Common::FileList::const_iterator c = chars.begin(); c != chars.end(); ++c) {
		if (!Common::FilePath::getExtension(*c).equalsIgnoreCase(".bic"))
			continue;

		characters.push_back(Common::FilePath::getStem(*c));
	}
}

void Game::getModules(std::vector<Common::UString> &modules) {
	modules.clear();

	Common::UString moduleDir = ConfigMan.getString("NWN_extraModuleDir");
	if (moduleDir.empty())
		return;

	Common::FileList mods;
	mods.addDirectory(moduleDir);

	for (Common::FileList::const_iterator m = mods.begin(); m != mods.end(); ++m) {
		if (!Common::FilePath::getExtension(*m).equalsIgnoreCase(".mod"))
			continue;

		modules.push_back(Common::FilePath::getStem(*m));
	}

	std::sort(modules.begin(), modules.end(), Common::UString::iless());
}

bool Game::hasModule(Common::UString &module) {
	const Common::UString nwmFile = module + ".nwm";
	const Common::UString modFile = module + ".mod";

	if (ResMan.hasArchive(module + ".nwm")) {
		module = nwmFile;
		return true;
	}

	if (ResMan.hasArchive(module + ".mod")) {
		module = modFile;
		return true;
	}

	return false;
}

} // End of namespace NWN

} // End of namespace Engines
