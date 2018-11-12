#include "main.h"

Bool PluginStart()
{
	ApplicationOutput("Hello C4D!");
	DiagnosticOutput("Hello VS!");

	return true;
}

void PluginEnd()
{
}

Bool PluginMessage(Int32 id, void *data)
{
	return true;
}
