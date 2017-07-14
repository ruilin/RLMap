package com.mapswithme.maps.search;

/**
 * Native search will return results via this interface.
 */
@SuppressWarnings("unused")
public interface NativeSearchListener
{
  /**
   * @param results Search results.
   * @param timestamp Timestamp of search request.
   */
  void onResultsUpdate(SearchResult[] results, long timestamp, boolean isHotel);

  /**
   * @param timestamp Timestamp of search request.
   */
  void onResultsEnd(long timestamp);
}
