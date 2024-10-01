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
        
}