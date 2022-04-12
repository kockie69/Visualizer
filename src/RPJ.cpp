#include "RPJ.hpp"
Plugin *pluginInstance;
void init(Plugin *p) {
    pluginInstance = p;
    p->addModel(modelVisualizer);
    //p->addModel(modelVisualizerFull);
    p->addModel(modelTest);
}
