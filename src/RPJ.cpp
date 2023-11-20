#include "RPJ.hpp"
Plugin *pluginInstance;
void init(Plugin *p) {
    pluginInstance = p;
	p->addModel(modelWindowedLFMModule);
	p->addModel(modelEmbeddedLFMModule);
}
