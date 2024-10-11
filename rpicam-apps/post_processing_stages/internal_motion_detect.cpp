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
    this->motion_noise_ = params.get<int>("motion_noise", 5);
    this->motion_threshold_ = params.get<int>("motion_threshold", 10);
    this->motion_image_ = params.get<char*>("motion_image", "make.pgm");
    this->motion_initframes_ = params.get<int>("motion_initframes", 0);
    this->motion_startframes_ = params.get<int>("motion_startframes", 5);
    this->motion_stopframes_ = params.get<int>("motion_stopframes", 50);
    this->motion_pipe_ = params.get<char*>("motion_pipe", "/var/www/FIFO1");
    this->motion_file_ = params.get<int>("motion_file", 0);
}

void internalMotionDetectStage::Configure()
{
    //  Configured variables that will be needed for motion detection

    // Setting up the low res stream for us to use
    stream_ = nullptr;
    if(app_ -> StillStream()){
        return;
    }

    stream_ = app_ -> LoresStream();
    if(!stream_){
        throw std::runtime_error("internalMotionDetectStage: no low resolution stream");
    }
	low_res_info_ = app_->GetStreamInfo(stream_);

}

void internalMotionDetectStage::Process(CompletedRequestPtr &completed_request)
{
    // Motion detection stuff
}

void internalMotionDetectStage::Start(){
    // Empty for now as not required
}

void internalMotionDetectStage::Stop(){
    // Empty for now as not required
}

void internalMotionDetectStage::TearDown(){
    // To be done with configure
    if(stream_){
        app_->ReleaseStream(stream_);
        stream_ = nullptr;
    }

    if(low_res_info_){
        delete low_res_info_;
        low_res_info_ = nullptr;
    }
}

static PostProcessingStage *Create(RPiCamApp *app)
{
	return new internalMotionDetectStage(app);
}

static RegisterStage reg(NAME, &Create);
