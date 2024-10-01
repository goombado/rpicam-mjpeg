// Remember to add to meson.build after implementing this file.

#include "internal_motion_detect.hpp"

#define NAME "internal_motion_detect"

char const *internalMotionDetectStage::Name() const
{
    return NAME;
}

void internalMotionDetectStage::Read(boost::property_tree::ptree const &params)
{
    // Read the parameters from the JSON file
    // Note: the second value is the default value if the parameter is not found
    this->motion_noise = params.get<int>("motion_noise", 5);
    this->motion_threshold = params.get<int>("motion_threshold", 10);
    this->motion_image = params.get<char*>("motion_image", "make.pgm");
    this->motion_initframes = params.get<int>("motion_initframes", 0);
    this->motion_startframes = params.get<int>("motion_startframes", 5);
    this->motion_stopframes = params.get<int>("motion_stopframes", 50);
    this->motion_pipe = params.get<char*>("motion_pipe", "/var/www/FIFO1");
    this->motion_file = params.get<int>("motion_file", 0);
}

void internalMotionDetectStage::Start(){
    // Empty for now as not required
}

void internalMotionDetectStage::Stop(){
    // Empty for now as not required
}

void internalMotionDetectStage::TearDown(){
    // To be done with configure
}

static PostProcessingStage *Create(RPiCamApp *app)
{
	return new internalMotionDetectStage(app);
}

static RegisterStage reg(NAME, &Create);
