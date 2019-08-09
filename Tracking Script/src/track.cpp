//Ahmed Kamal
//July 2019
//Colby College Computational Physiology and Optimization Lab

#include <cstdio>
#include <opencv2/opencv.hpp>

using namespace cv;

int main(int argc, char *argv[]) {

	cv::VideoCapture *capdev;

	// open the video device
	capdev = new cv::VideoCapture("../data/video.mp4");
	if( !capdev->isOpened() ) {
		printf("Unable to open video\n");
		return(-1);
	}

	//Get the size of the feed
	cv::Size refS( (int) capdev->get(cv::CAP_PROP_FRAME_WIDTH ),
		       (int) capdev->get(cv::CAP_PROP_FRAME_HEIGHT));
	//Print the size
	printf("Expected size: %d %d\n", refS.width, refS.height);

	cv::namedWindow("Stream", 1); // identifies a window
	cv::Mat frame; //The star of the show
	std::vector<cv::Mat> spl; //Holds the RGB channels of the image 
	cv::Mat lFrame; //Labeled Frame
	cv::Mat lFrameStats; //Stats for regions in the labeled component image
	cv::Mat lFrameCentroids; //The centroids of the labeled regions
	cv::Mat path = Mat(0,2,CV_64F); //A matrix for the set of points in the path travelled
	cv::Mat temp = Mat(1,2,CV_64F);	//A temporary matrix for holding points
	double lastX = -1; //The last known X position
	double lastY = 0; //The last known Y position
	double distance= 0; //The total distance travelled
	char buffer [70]; //Useful for printing

	for(;;) {
		*capdev >> frame; // get a new frame from the camera, treat as a stream

		if( frame.empty() ) {
		  printf("frame is empty\n");
		  break;
		}
		
		//Split the image into RGB channels
		split(frame,spl);
		//Scale the red pixels, so instead of returning the 'absolute redness' in an image, we return the 'relative redness'
		//i.e. White goes to 0, but red stays red
		spl[2] = spl[2]-((spl[0]+spl[1])/2);
		//Show the relative redness

		//Process the image to isolate the reddest areas as foreground areas in a binary image
		threshold(spl[2],spl[2],100,255,0);
		
		//Morph the image to get rid of spots
		erode(spl[2], spl[2], cv::Mat(),cv::Point(-1,-1),1);
		dilate(spl[2], spl[2], cv::Mat(),cv::Point(-1,-1),13);
		erode(spl[2], spl[2], cv::Mat(),cv::Point(-1,-1),12);

		//Run connected components on the image
		int numComponents = connectedComponentsWithStats(spl[2], lFrame,lFrameStats,lFrameCentroids, 8);

		/* Not needed apart from debugging
		//Convert the label image to a more friendly format, incase we want to process it further (color individual regions
		lFrame.convertTo(lFrame, CV_8U);

		//Color the components
		for(int r = 0; r < lFrame.rows; ++r){
			for(int c = 0; c < lFrame.cols; ++c){
				lFrame.at<uchar>(r,c) = lFrame.at<uchar>(r,c)*100;
		 	}
		}
		*/

		//Run the following if and only if we detect one foreground component
		//This acts as a safety measure - noisy connected component frames and frames with no connected components are ignored
		if(numComponents==2){

			//Store the new X and Y to the path
			temp.at<double>(0,0) = (int)lFrameCentroids.at<double>(1,0);
			temp.at<double>(0,1) = (int)lFrameCentroids.at<double>(1,1);
			path.push_back(temp);

			//Determine our scale (The number 7.5 is the diameter of the circular red sticker I have been using)
			int widthOfSticker = lFrameStats.at<int>(1,2);
			double cmPerPixel = 7.5/widthOfSticker;

			//If we have information on a previous position, compute and add the distance travelled since the last frame
			if(lastX != -1){
				double distanceX = lastX - temp.at<double>(0,0);
				double distanceY = lastY - temp.at<double>(0,1);
				//Use += for distance, = for displacement
				distance = sqrt(distanceX*distanceX + distanceY*distanceY)*cmPerPixel;
			}

			//DISPLACEMENT CODE BLOCK
			//Update the last known position only once
			if(lastX == -1){
				lastX = temp.at<double>(0,0);
				lastY = temp.at<double>(0,1);
			}

			//DISTANCE CODE BLOCK
			//Update the last known position
			//lastX = temp.at<double>(0,0);
			//lastY = temp.at<double>(0,1);
			

			//Print out information
			sprintf(buffer,"Width in pixels: %d\tcm/pixel: %.2f\tDistance travelled: %.2fcm",widthOfSticker,cmPerPixel,distance);
			std::cout << "\33[2K\r" << buffer << std::flush;

			sprintf(buffer,"Dist: %.2fcm",distance);
			//YOU WERE ABOUT TO ADD TEXT TO THE IMAGE GIVING THE CURRENT DISTANCE
			//ALSO WANT TO WRITE DOWN THE TIME/FRAME, DISTANCE-FROM-ORIGIN TO A CSV FILE
			cv::putText(frame, buffer, cv::Point(lastX,lastY), cv::FONT_HERSHEY_SIMPLEX, 1.0, CV_RGB(0,255,0), 2.0);

			//Possibly compute distance parallel to the black track

		}
		
		//Draw the path on the image
		if(path.rows>0){
			for(int i = 0;i<path.rows;i++){
				double r = (int) path.at<double>(i,0);
				double c = (int) path.at<double>(i,1);
				int R = (int)r;
				int C = (int)c;
				for(int j = -9;j<=9;j++){
					for(int k = -9;k<=9;k++){
						frame.at<Vec3b>(C+k,R+j)[0] = 0;
						frame.at<Vec3b>(C+k,R+j)[1] = 255;
						frame.at<Vec3b>(C+k,R+j)[2] = 0;
					}
				}
			}
		}

		//Show the image
		cv::imshow("Stream", frame);
		char keyPressed = cv::waitKey(100000);
		if(keyPressed=='q'){
			break;
		}

	}

	// terminate the video capture
	printf("\nTerminating\n");
	delete capdev;

	return(0);
}
