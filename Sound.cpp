#define WINDOBJ_MAIN

#include <WindObj/Private.h>

MidiOutCable::~MidiOutCable() {delete [] m_name;}

static MidiOutCables g_midiOutCables;

MidiOutCables::MidiOutCables() : m_nCables(0), m_cable(NULL) {}

MidiOutCables::~MidiOutCables()
{
  delete [] m_cable;
}

WO_EXPORT const MidiOutCables *GetMidiOutCables()
{
  if (!g_midiOutCables.m_nCables)
  {
    g_midiOutCables.m_nCables = midiOutGetNumDevs() + 1;
    g_midiOutCables.m_cable = new MidiOutCable[g_midiOutCables.m_nCables];
    int i, n;
    MIDIOUTCAPS MOC;
    for (i = 0; i < g_midiOutCables.m_nCables - 1; i++)
    {
      midiOutGetDevCaps(i, &MOC, sizeof(MIDIOUTCAPS));
      n = strlen(MOC.szPname);
      strcpy(g_midiOutCables.m_cable[i].m_name = new char[n + 1], MOC.szPname);
    }
    midiOutGetDevCaps(MIDI_MAPPER, &MOC, sizeof(MIDIOUTCAPS));
    n = strlen(MOC.szPname);
    strcpy(g_midiOutCables.m_cable[i].m_name = new char[n + 1], MOC.szPname);
  }
  return &g_midiOutCables;
}
