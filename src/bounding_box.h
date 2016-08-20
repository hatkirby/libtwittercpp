#ifndef BOUNDING_BOX_H_75D2077D
#define BOUNDING_BOX_H_75D2077D

namespace twitter {
  
  class coordinate {
  public:
    
    coordinate(int degrees, int minutes = 0, int seconds = 0) :
      _degrees(degrees),
      _minutes(minutes),
      _seconds(seconds)
    {
    }
    
    int getDegrees() const
    {
      return _degrees;
    }
    
    int getMinutes() const
    {
      return _minutes;
    }
    
    int getSeconds() const
    {
      return _seconds;
    }
    
    operator double() const
    {
      return (double)_degrees + ((double)_minutes / (double)60) + ((double)_seconds / (double)3600);
    }
    
  private:
    
    int _degrees;
    int _minutes;
    int _seconds;
  };
  
  class bounding_box {
  public:
    
    bounding_box(
      coordinate south_west_long,
      coordinate south_west_lat,
      coordinate north_east_long,
      coordinate north_east_lat) :
        _south_west_long(south_west_long),
        _south_west_lat(south_west_lat),
        _north_east_long(north_east_long),
        _north_east_lat(north_east_lat)
    {
    }
    
    const coordinate& getSouthWestLongitude() const
    {
      return _south_west_long;
    }
    
    const coordinate& getSouthWestLatitude() const
    {
      return _south_west_lat;
    }
    
    const coordinate& getNorthEastLongitude() const
    {
      return _north_east_long;
    }
    
    const coordinate& getNorthEastLatitude() const
    {
      return _north_east_lat;
    }
    
  private:
    
    coordinate _south_west_long;
    coordinate _south_west_lat;
    coordinate _north_east_long;
    coordinate _north_east_lat;
  };
  
}

#endif /* end of include guard: BOUNDING_BOX_H_75D2077D */
