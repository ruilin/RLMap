@protocol MWMSearchTabbedViewProtocol <NSObject>

@required

- (void)searchText:(NSString *)text forInputLocale:(NSString *)locale;
- (void)dismissKeyboard;

@end
