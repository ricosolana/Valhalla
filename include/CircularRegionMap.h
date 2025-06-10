#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdint>

template<typename T>
class CircularRegionMap {
private:
    //T* data;
    std::vector<T> data;
    //int width, height;
    int diameter; //is also height (or moreso, diameter)
    int validCount;
    
    // Helper function to check if a position is valid (within circle and not corner)
    bool isValidPosition(int x, int y) const {
        // Calculate center coordinates
        int centerX = diameter / 2;
        int centerY = diameter / 2;
        
        // Check if point is within circle
        double distanceSquared = (x - centerX) * (x - centerX) + (y - centerY) * (y - centerY); //pow...
        double radiusSquared = (diameter / 2.0) * (diameter / 2.0); //pow...
        
        return distanceSquared <= radiusSquared && !isCorner(x, y);
    }
    
    // Helper function to check if a position is a corner
    bool isCorner(int x, int y) const {
        int margin = diameter / 4; // Quarter of max dimension
        
        return (x < margin || x >= diameter - margin ||
                y < margin || y >= diameter - margin);
    }
    
public:
    CircularRegionMap(int w) : diameter(w) {
        // Count valid positions
        validCount = 0;
        for (int y = 0; y < diameter; ++y) {
            for (int x = 0; x < diameter; ++x) {
                if (isValidPosition(x, y)) {
                    validCount++;
                }
            }
        }
    }
    
    ~CircularRegionMap() {
        //stuff here...
    }
    
    // Convert XY coordinates to index
    int getIndex(int x, int y) const {
        if (!isValidPosition(x, y)) {
            throw std::out_of_range("Coordinates outside valid region");
        }
        
        // Calculate relative position from center
        int centerX = diameter / 2;
        int centerY = diameter / 2;
        int relX = x - centerX;
        int relY = y - centerY;
        
        // Use polar angle and distance for ordering
        //double angle = std::atan2(relY, relX); //TODO approximate atan
        //double distance = sqrt(pow(relX, 2) + pow(relY, 2));
        //
        //// Map to index based on angle and distance
        //int index = static_cast<int>((angle / (2 * M_PI) * validCount) +
        //                           (distance / (diameter/2.0) * validCount));
        assert(false);
        //return index % validCount;
        return 0;
    }
    
    T& operator[](int index) {
        if (index < 0 || index >= validCount) {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }
};