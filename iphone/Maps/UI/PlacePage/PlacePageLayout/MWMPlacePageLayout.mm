#import "MWMPlacePageLayout.h"
#import "MWMBookmarkCell.h"
#import "MWMCircularProgress.h"
#import "MWMUGCCommentCell.h"
#import "MWMOpeningHoursLayoutHelper.h"
#import "MWMPPPreviewLayoutHelper.h"
#import "MWMPPReviewCell.h"
#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageCellUpdateProtocol.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageLayoutImpl.h"
#import "MWMPlacePageRegularCell.h"
#import "MWMiPadPlacePageLayoutImpl.h"
#import "MWMiPhonePlacePageLayoutImpl.h"
#import "SwiftBridge.h"

#include "ugc/types.hpp"

#include "storage/storage.hpp"

#include "std/array.hpp"
#include "std/map.hpp"

namespace
{
using place_page::MetainfoRows;

map<MetainfoRows, Class> const kMetaInfoCells = {
    {MetainfoRows::Website, [MWMPlacePageLinkCell class]},
    {MetainfoRows::Address, [MWMPlacePageInfoCell class]},
    {MetainfoRows::Email, [MWMPlacePageLinkCell class]},
    {MetainfoRows::Phone, [MWMPlacePageLinkCell class]},
    {MetainfoRows::Cuisine, [MWMPlacePageInfoCell class]},
    {MetainfoRows::Operator, [MWMPlacePageInfoCell class]},
    {MetainfoRows::Coordinate, [MWMPlacePageInfoCell class]},
    {MetainfoRows::Internet, [MWMPlacePageInfoCell class]}};
}  // namespace

@interface MWMPlacePageLayout () <UITableViewDataSource,
                                 MWMPlacePageCellUpdateProtocol, MWMPlacePageViewUpdateProtocol>

@property(weak, nonatomic) MWMPlacePageData * data;

@property(weak, nonatomic) UIView * ownerView;
@property(weak, nonatomic)
    id<MWMPlacePageLayoutDelegate, MWMPlacePageButtonsProtocol, MWMActionBarProtocol>
        delegate;
@property(weak, nonatomic) id<MWMPlacePageLayoutDataSource> dataSource;
@property(nonatomic) IBOutlet MWMPPView * placePageView;

@property(nonatomic) MWMBookmarkCell * bookmarkCell;

@property(nonatomic) MWMPlacePageActionBar * actionBar;

@property(nonatomic) BOOL isPlacePageButtonsEnabled;
@property(nonatomic) BOOL isDownloaderViewShown;

@property(nonatomic) id<MWMPlacePageLayoutImpl> layoutImpl;

@property(nonatomic) MWMPPPreviewLayoutHelper * previewLayoutHelper;
@property(nonatomic) MWMOpeningHoursLayoutHelper * openingHoursLayoutHelper;

@end

@implementation MWMPlacePageLayout

- (instancetype)initWithOwnerView:(UIView *)view
                         delegate:(id<MWMPlacePageLayoutDelegate, MWMPlacePageButtonsProtocol,
                                      MWMActionBarProtocol>)delegate
                       dataSource:(id<MWMPlacePageLayoutDataSource>)dataSource
{
  self = [super init];
  if (self)
  {
    _ownerView = view;
    _delegate = delegate;
    _dataSource = dataSource;
    [[NSBundle mainBundle] loadNibNamed:[MWMPPView className] owner:self options:nil];
    [_placePageView layoutIfNeeded];
    _placePageView.delegate = self;
    auto const Impl = IPAD ? [MWMiPadPlacePageLayoutImpl class] : [MWMiPhonePlacePageLayoutImpl class];
    _layoutImpl = [[Impl alloc] initOwnerView:view placePageView:_placePageView delegate:delegate];

    if ([_layoutImpl respondsToSelector:@selector(setInitialTopBound:leftBound:)])
      [_layoutImpl setInitialTopBound:dataSource.topBound leftBound:dataSource.leftBound];

    auto tableView = _placePageView.tableView;
    _previewLayoutHelper = [[MWMPPPreviewLayoutHelper alloc]
                                                     initWithTableView:tableView];
    _openingHoursLayoutHelper = [[MWMOpeningHoursLayoutHelper alloc] initWithTableView:tableView];
    [self registerCells];
  }
  return self;
}

- (void)registerCells
{
  auto tv = self.placePageView.tableView;
  [tv registerWithCellClass:[MWMPlacePageButtonCell class]];
  [tv registerWithCellClass:[MWMBookmarkCell class]];
  [tv registerWithCellClass:[MWMPPHotelDescriptionCell class]];
  [tv registerWithCellClass:[MWMPPHotelCarouselCell class]];
  [tv registerWithCellClass:[MWMPPViatorCarouselCell class]];
  [tv registerWithCellClass:[MWMPPReviewHeaderCell class]];
  [tv registerWithCellClass:[MWMPPReviewCell class]];
  [tv registerWithCellClass:[MWMPPFacilityCell class]];
  [tv registerWithCellClass:[MWMPlacePageTaxiCell class]];
  [tv registerWithCellClass:[MWMUGCSelectImpressionCell class]];
  [tv registerWithCellClass:[MWMUGCCommentCell class]];

  // Register all meta info cells.
  for (auto const & pair : kMetaInfoCells)
    [tv registerWithCellClass:pair.second];
}

- (void)layoutWithSize:(CGSize const &)size
{
  [self.layoutImpl onScreenResize:size];
}

- (UIView *)shareAnchor { return self.actionBar.shareAnchor; }
- (void)showWithData:(MWMPlacePageData *)data
{
  self.isPlacePageButtonsEnabled = YES;
  self.data = data;

  data.sectionsAreReadyCallback = ^(NSRange const & range, MWMPlacePageData * d, BOOL isSection) {
    if (![self.data isEqual:d])
      return;

    auto tv = self.placePageView.tableView;
    if (isSection) {
      [tv insertSections:[NSIndexSet indexSetWithIndexesInRange:range]
                                withRowAnimation:UITableViewRowAnimationAutomatic];
    }
    else
    {
      NSMutableArray<NSIndexPath *> * indexPaths = [@[] mutableCopy];
      for (auto i = 1; i < range.length + 1; i++)
        [indexPaths addObject:[NSIndexPath indexPathForRow:i inSection:range.location]];

      [tv insertRowsAtIndexPaths:indexPaths withRowAnimation:UITableViewRowAnimationAutomatic];
    }
  };

  data.bannerIsReadyCallback = ^{
    [self.previewLayoutHelper insertRowAtTheEnd];
  };
  self.bookmarkCell = nil;

  [self.actionBar configureWithData:static_cast<id<MWMActionBarSharedData>>(data)];
  [self.previewLayoutHelper configWithData:data];
  [self.openingHoursLayoutHelper configWithData:data];
  if ([self.layoutImpl respondsToSelector:@selector(setPreviewLayoutHelper:)])
    [self.layoutImpl setPreviewLayoutHelper:self.previewLayoutHelper];

  [self.placePageView.tableView reloadData];
  [self.layoutImpl onShow];

  dispatch_async(dispatch_get_main_queue(), ^{
    [data fillOnlineBookingSections];
    [data fillOnlineViatorSection];
  });
}

- (void)rotateDirectionArrowToAngle:(CGFloat)angle
{
  [self.previewLayoutHelper rotateDirectionArrowToAngle:angle];
}

- (void)setDistanceToObject:(NSString *)distance
{
  [self.previewLayoutHelper setDistanceToObject:distance];
}

- (void)setSpeedAndAltitude:(NSString *)speedAndAltitude
{
  [self.previewLayoutHelper setSpeedAndAltitude:speedAndAltitude];
}

- (MWMPlacePageActionBar *)actionBar
{
  if (!_actionBar)
  {
    _actionBar = [MWMPlacePageActionBar actionBarWithDelegate:self.delegate];
    self.layoutImpl.actionBar = _actionBar;
  }
  return _actionBar;
}

- (void)close
{
  [self.layoutImpl onClose];
}

- (void)mwm_refreshUI
{
  [self.placePageView mwm_refreshUI];
  [self.actionBar mwm_refreshUI];
}

- (void)reloadBookmarkSection:(BOOL)isBookmark
{
  auto tv = self.placePageView.tableView;
  NSIndexSet * set =
      [NSIndexSet indexSetWithIndex:static_cast<NSInteger>(place_page::Sections::Bookmark)];

  if (isBookmark)
  {
    if (self.bookmarkCell)
      [tv reloadSections:set withRowAnimation:UITableViewRowAnimationAutomatic];
    else
      [tv insertSections:set withRowAnimation:UITableViewRowAnimationAutomatic];
  }
  else
  {
    [tv deleteSections:set withRowAnimation:UITableViewRowAnimationAutomatic];
    self.bookmarkCell = nil;
  }
}

#pragma mark - Downloader event

- (void)processDownloaderEventWithStatus:(storage::NodeStatus)status progress:(CGFloat)progress
{
  auto data = self.data;
  if (!data)
    return;

  using namespace storage;
  auto const & sections = data.sections;
  switch (status)
  {
  case NodeStatus::OnDiskOutOfDate:
  case NodeStatus::Undefined:
  {
    self.isPlacePageButtonsEnabled = NO;
    auto const it = find(sections.begin(), sections.end(), place_page::Sections::Buttons);
    if (it != sections.end())
    {
      [self.placePageView.tableView
            reloadSections:[NSIndexSet indexSetWithIndex:distance(sections.begin(), it)]
          withRowAnimation:UITableViewRowAnimationAutomatic];
    }
    self.actionBar.isAreaNotDownloaded = NO;
    break;
  }
  case NodeStatus::Downloading:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingProgress = progress;
    break;
  }
  case NodeStatus::InQueue:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateSpinner;
    break;
  }
  case NodeStatus::Error:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateFailed;
    break;
  }
  case NodeStatus::OnDisk:
  {
    self.isPlacePageButtonsEnabled = YES;
    auto const it = find(sections.begin(), sections.end(), place_page::Sections::Buttons);
    if (it != sections.end())
    {
      [self.placePageView.tableView
            reloadSections:[NSIndexSet indexSetWithIndex:distance(sections.begin(), it)]
          withRowAnimation:UITableViewRowAnimationAutomatic];
    }
    self.actionBar.isAreaNotDownloaded = NO;
    break;
  }
  case NodeStatus::Partly:
  case NodeStatus::NotDownloaded:
  {
    self.isPlacePageButtonsEnabled = NO;
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateNormal;
    break;
  }
  }
}

#pragma mark - iPad only

- (void)updateTopBound
{
  if (![self.layoutImpl respondsToSelector:@selector(updateLayoutWithTopBound:)])
  {
    NSAssert(!IPAD, @"iPad layout must implement updateLayoutWithTopBound:!");
    return;
  }

  [self.layoutImpl updateLayoutWithTopBound:self.dataSource.topBound];
}

- (void)updateLeftBound
{
  if (![self.layoutImpl respondsToSelector:@selector(updateLayoutWithLeftBound:)])
  {
    NSAssert(!IPAD, @"iPad layout must implement updateLayoutWithLeftBound:!");
    return;
  }

  [self.layoutImpl updateLayoutWithLeftBound:self.dataSource.leftBound];
}

#pragma mark - UITableViewDelegate & UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  auto data = self.data;
  if (!data)
    return 0;
  return data.sections.size();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  using namespace place_page;

  auto data = self.data;
  if (!data)
    return 0;
  switch (data.sections[section])
  {
  case Sections::Bookmark: return 1;
  case Sections::Preview: return data.previewRows.size();
  case Sections::Viator: return data.viatorRows.size();
  case Sections::Metainfo: return data.metainfoRows.size();
  case Sections::Ad: return data.adRows.size();
  case Sections::Buttons: return data.buttonsRows.size();
  case Sections::HotelPhotos: return data.photosRows.size();
  case Sections::HotelDescription: return data.descriptionRows.size();
  case Sections::HotelFacilities: return data.hotelFacilitiesRows.size();
  case Sections::HotelReviews: return data.hotelReviewsRows.size();
  case Sections::UGC: return data.ugcRows.size();
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  using namespace place_page;

  auto data = self.data;
  if (!data)
    return [[UITableViewCell alloc] init];

  id<MWMPlacePageButtonsProtocol> delegate = self.delegate;
  switch (data.sections[indexPath.section])
  {
  case Sections::Preview:
  {
    return [self.previewLayoutHelper cellForRowAtIndexPath:indexPath withData:data];
  }
  case Sections::Bookmark:
  {
    Class cls = [MWMBookmarkCell class];
    auto c = static_cast<MWMBookmarkCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    [c configureWithText:data.bookmarkDescription
          updateCellDelegate:self
        editBookmarkDelegate:delegate
                      isHTML:data.isHTMLDescription];
    return c;
  }
  case Sections::Metainfo:
  {
    auto const row = data.metainfoRows[indexPath.row];
    switch (row)
    {
    case MetainfoRows::OpeningHours:
    case MetainfoRows::ExtendedOpeningHours:
      return [self.openingHoursLayoutHelper cellForRowAtIndexPath:indexPath];
    case MetainfoRows::Phone:
    case MetainfoRows::Address:
    case MetainfoRows::Website:
    case MetainfoRows::Email:
    case MetainfoRows::Cuisine:
    case MetainfoRows::Operator:
    case MetainfoRows::Internet:
    case MetainfoRows::Coordinate:
    {
      Class cls = kMetaInfoCells.at(row);
      auto c = static_cast<MWMPlacePageRegularCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configWithRow:row data:data];
      return c;
    }
    case MetainfoRows::LocalAdsCustomer:
    case MetainfoRows::LocalAdsCandidate:
    {
      Class cls = [MWMPlacePageButtonCell class];
      auto c = static_cast<MWMPlacePageButtonCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                               indexPath:indexPath]);
      [c configWithTitle:[data stringForRow:row] action:^{ [delegate openLocalAdsURL]; } isInsetButton:NO];
      return c;
    }
    }
  }
  case Sections::Ad:
  {
    Class cls = [MWMPlacePageTaxiCell class];
    auto c = static_cast<MWMPlacePageTaxiCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    auto const & taxiProviders = [data taxiProviders];
    NSAssert(!taxiProviders.empty(), @"TaxiProviders can not be empty");
    auto const & provider = taxiProviders.front();
    auto type = MWMPlacePageTaxiProviderTaxi;
    switch (provider)
    {
    case taxi::Provider::Uber: type = MWMPlacePageTaxiProviderUber; break;
    case taxi::Provider::Yandex: type = MWMPlacePageTaxiProviderYandex; break;
    }
    [c configWithType:type delegate:delegate];
    return c;
  }
  case Sections::Buttons:
  {
    Class cls = [MWMPlacePageButtonCell class];
    auto c = static_cast<MWMPlacePageButtonCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    auto const row = data.buttonsRows[indexPath.row];

    [c configForRow:row withAction:^{
      switch (row)
      {
        case ButtonsRows::AddPlace: [delegate addPlace]; break;
        case ButtonsRows::EditPlace: [delegate editPlace]; break;
        case ButtonsRows::AddBusiness: [delegate addBusiness]; break;
        case ButtonsRows::HotelDescription: [delegate book:YES]; break;
        case ButtonsRows::Other: NSAssert(false, @"Incorrect row");
      }
    }];
    
    if (row != ButtonsRows::HotelDescription)
      [c setEnabled:self.isPlacePageButtonsEnabled];
    else
      [c setEnabled:YES];

    return c;
  }
  case Sections::Viator:
  {
    Class cls = [MWMPPViatorCarouselCell class];
    auto c = static_cast<MWMPPViatorCarouselCell *>(
        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    [c configWith:data.viatorItems delegate:delegate];
    return c;
  }
  case Sections::HotelPhotos:
  {
    Class cls = [MWMPPHotelCarouselCell class];
    auto c = static_cast<MWMPPHotelCarouselCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
    [c configWith:data.photos delegate:delegate];
    return c;
  }
  case Sections::HotelFacilities:
  {
    auto const row = data.hotelFacilitiesRows[indexPath.row];
    switch (row)
    {
    case HotelFacilitiesRow::Regular:
    {
      Class cls = [MWMPPFacilityCell class];
      auto c = static_cast<MWMPPFacilityCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configWith:@(data.facilities[indexPath.row].m_name.c_str())];
      return c;
    }
    case HotelFacilitiesRow::ShowMore:
      Class cls = [MWMPlacePageButtonCell class];
      auto c = static_cast<MWMPlacePageButtonCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configWithTitle:L(@"booking_show_more") action:^{ [delegate showAllFacilities]; } isInsetButton:NO];
      return c;
    }
  }
  case Sections::HotelReviews:
  {
    auto const row = data.hotelReviewsRows[indexPath.row];
    switch (row)
    {
    case HotelReviewsRow::Header:
    {
      Class cls = [MWMPPReviewHeaderCell class];
      auto c = static_cast<MWMPPReviewHeaderCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configWith:data.bookingRating numberOfReviews:data.numberOfHotelReviews];
      return c;
    }
    case HotelReviewsRow::Regular:
    {
      Class cls = [MWMPPReviewCell class];
      auto c = static_cast<MWMPPReviewCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configWithReview:data.hotelReviews[indexPath.row - 1]];
      return c;
    }
    case HotelReviewsRow::ShowMore:
    {
      Class cls = [MWMPlacePageButtonCell class];
      auto c = static_cast<MWMPlacePageButtonCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);

      [c configWithTitle:L(@"reviews_on_bookingcom") action:^{ [delegate showAllReviews]; } isInsetButton:NO];
      return c;
    }
    }
  }
  case Sections::HotelDescription:
  {
    auto const row = data.descriptionRows[indexPath.row];
    switch (row)
    {
    case HotelDescriptionRow::Regular:
    {
      Class cls = [MWMPPHotelDescriptionCell class];
      auto c = static_cast<MWMPPHotelDescriptionCell *>(
                                                        [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configWith:data.hotelDescription delegate:self];
      return c;
    }
    case HotelDescriptionRow::ShowMore:
    {
      Class cls = [MWMPlacePageButtonCell class];
      auto c = static_cast<MWMPlacePageButtonCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configWithTitle:L(@"more_on_bookingcom") action:^{ [delegate book:YES];; } isInsetButton:NO];
      return c;
    }
    }
  }
  case Sections::UGC:
  {
    auto const row = data.ugcRows[indexPath.row];
    switch (row)
    {
      case UGCRow::SelectImpression:
      {
        Class cls = [MWMUGCSelectImpressionCell class];
        auto c = static_cast<MWMUGCSelectImpressionCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithDelegate:delegate];
        return c;
      }
      case UGCRow::Comment:
      {
        Class cls = [MWMUGCCommentCell class];
        auto c = static_cast<MWMUGCCommentCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithReview:data.ugcReviews[indexPath.row - 1]];
        return c;
      }
      case UGCRow::ShowMore:
      {
        Class cls = [MWMPlacePageButtonCell class];
        auto c = static_cast<MWMPlacePageButtonCell *>([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithTitle:L(@"ugc_more_reviews") action:^{} isInsetButton:NO];
        return c;
      }
    }
  }
  }
}

#pragma mark - MWMPlacePageCellUpdateProtocol

- (void)cellUpdated
{
  auto const update = @selector(update);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:update object:nil];
  [self performSelector:update withObject:nil afterDelay:0.1];
}

- (void)update
{
  auto data = self.data;
  if (data)
    [self.placePageView.tableView refresh];
}

#pragma mark - MWMPlacePageViewUpdateProtocol

- (void)updateWithHeight:(CGFloat)height
{
  auto const sel = @selector(updatePlacePageHeight);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:sel object:nil];
  [self performSelector:sel withObject:nil afterDelay:0.1];
}

- (void)updatePlacePageHeight
{
  [self.layoutImpl onUpdatePlacePageWithHeight:self.placePageView.tableView.contentSize.height];
}

#pragma mark - Properties

- (void)setData:(MWMPlacePageData *)data
{
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  _data = data;
}

@end
