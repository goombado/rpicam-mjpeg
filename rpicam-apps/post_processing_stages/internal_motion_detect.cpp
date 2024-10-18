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
    this->motion_image_ = params.get<std::string>("motion_image", "test1GrayScalePGM.pgm");
    this->motion_initframes_ = params.get<int>("motion_initframes", 0);
    this->motion_startframes_ = params.get<int>("motion_startframes", 5);
    this->motion_stopframes_ = params.get<int>("motion_stopframes", 50);
    this->motion_pipe_ = params.get<std::string>("motion_pipe", "/var/www/FIFO1");
    this->motion_file_ = params.get<int>("motion_file", 0);
}

void internalMotionDetectStage::Configure(){
    //  Configured variables that will be needed for motion detection

    // Setting up the low res stream for us to use
    stream_ = nullptr;

    stream_ = app_ -> LoresStream();
    if(!stream_){
        throw std::runtime_error("internalMotionDetectStage: no low resolution stream");
    }
	low_res_info_ = app_->GetStreamInfo(stream_);

}

bool internalMotionDetectStage::Process(CompletedRequestPtr &completed_request){
    // Motion detection stuff
    if(!stream_){
        return false;
    } 

    frame_counter = 0;
    motion_frame_counter = 0;
    no_motion_counter = 0;

    // Load grayscale image 
    cv::Mat mask = cv::imread(motion_image_, cv::IMREAD_GRAYSCALE);
    if(mask.empty()){
        throw std::runtime_error("internalMotionDetectStage: could not load mask image");
        return false;
    }

    //Convert current frame to openCV mat
    BufferReadSync r(app_, completed_request->buffers[stream_]);
    libcamera::Span<uint8_t> buffer = r.Get()[0];
    uint8_t *ptr = (uint8_t *)buffer.data();
    cv::Mat current_frame(low_res_info_.height, low_res_info_.width, CV_8UC1, ptr);

    current_frame_ = current_frame.clone();
    // Check if the initial number of frames has been passed
    if (frame_counter < motion_initframes_){
        prev_frame = current_frame_.clone();
        frame_counter++;
        return false;
    }

    // Calculate the difference between the current frame and the previous frame
    cv::Mat diff_frame;
    cv::absdiff(prev_frame, current_frame_, diff_frame);

    // Apply the motion mask
    cv::Mat masked_diff_frame;
    cv::bitwise_and(diff_frame, mask, masked_diff_frame);

    // Filtering noise according to our motion_noise_ parameter
    cv::threshold(masked_diff_frame, masked_diff_frame, motion_noise_, 255, cv::THRESH_TOZERO);

    //Count number of pixels above motion_threshold_
    int motion_pixels = cv::countNonZero(masked_diff_frame > motion_threshold_);

    if(motion_pixels > 0){
        motion_frame_counter++;
        no_motion_counter = 0;

        if(motion_frame_counter > motion_startframes_){
            // Motion detected
            std::cout << "Motion detected" << std::endl;
            if(motion_file_){
                std::ofstream motion_pipe(motion_pipe_);
                motion_pipe << "1";
                motion_pipe.close();
            }
        }
    } else {
        no_motion_counter++;
        motion_frame_counter = 0;

        if(no_motion_counter > motion_stopframes_){
            std::cout << "No motion detected" << std::endl;
            // No motion detected
            if(motion_file_){
                std::ofstream motion_pipe(motion_pipe_);
                motion_pipe << "0";
                motion_pipe.close();
            }
        }
    }

    // Update the previous frame
    prev_frame = current_frame.clone();
	return false;
    
}

void internalMotionDetectStage::Start(){
    // Empty for now as not required
}

void internalMotionDetectStage::Stop(){
    // Empty for now as not required
}   

void internalMotionDetectStage::Teardown(){
    // To be done with configure
}

static PostProcessingStage *Create(RPiCamApp *app)
{
	return new internalMotionDetectStage(app);
}

static RegisterStage reg(NAME, &Create);
