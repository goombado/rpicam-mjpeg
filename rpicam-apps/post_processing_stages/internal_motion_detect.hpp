#include <libcamera/stream.h> 

#include "core/rpicam_app.hpp"

#include "post_processing_stages/post_processing_stage.hpp"

using Stream = libcamera::Stream;

class internalMotionDetectStage : public PostProcessingStage{
    public:
        internalMotionDetectStage(RPiCamApp *app) : PostProcessingStage(app) {}

        // returns the name of the stage
        char const *Name() const override;

        // Reads the stage's configuration parameters from a provided JSON file
        void Read(boost::property_tree::ptree const &params) override;

        // Called after the camera has been configured and check that the stage has the necessary streams
        void Configure() override;

        // Called when the camera starts
        // Can be empty for stages that don't need to configure the camera
        void Start() override;

        // Presents completed camera requests for post processing, where to implement pxiel manipulation and image analysis
        // Returns true if the post processing framework should not deliver this request to the app  
        bool Process(CompletedRequestPtr &completed_request) override;    

        // Called when the camera stops, used to shut down any processing on asynchronous threads.
        void Stop() override;

        // Use this as a destructor for configure() to deallocate resources
        void TearDown() override;

    private:
        int motion_noise; // If difference between frames is less than this, ignore it
        int motion_threshold; // If difference between frames is greater than this, consider it motion
        char* motion_image; // Grayscale binary mask image
        int motion_initframes; // Number of frames to wait at the start before checking for motion
        int motion_startframes; // If motion is detected for this amount of frames consectutively, then it is considered motion
        int motion_stopframes; // Once motion is no longer detected for this amount of frames, then we stop the motion event
        char* motion_pipe; // Path to the named pipe to write motion events to
        int motion_file; 
}