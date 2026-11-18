#define main kfx_launcher_main
#include "../packaging/Launcher.m"
#undef main
#include <assert.h>
int main(){@autoreleasepool{
 NSMutableString *text=[@"[audio]\nmusic_volume = 75\n[keys]\n  MoveUp = { code = \"W\", mods = \"SHIFT\", controller_buttons = \"LS_UP\" }\n[other]\nkeep = 42\n" mutableCopy];
 assert(ApplyControllerBindings(text,@{@"MoveUp":@"DPAD_UP",@"LeftClick":@"R2|A"},NULL));
 assert([text containsString:@"code = \"W\", mods = \"SHIFT\", controller_buttons = \"DPAD_UP\""]);
 assert([text containsString:@"music_volume = 75"] && [text containsString:@"keep = 42"]);
 [text writeToFile:@"/tmp/kfx-bindings-test.toml" atomically:YES encoding:NSUTF8StringEncoding error:NULL];
 NSString *legacyPath=@"/tmp/kfx-legacy-metadata-test.sav";
 struct FileChunkHeader header={sizeof(struct CatalogueEntry),SGC_InfoBlock,1};struct CatalogueEntry entry={0};entry.level_num=7;strcpy(entry.textname,"Legacy Test");strcpy(entry.campaign_name,"Original campaign");
 FILE *file=fopen(legacyPath.UTF8String,"wb");assert(file);fwrite(&header,sizeof(header),1,file);fwrite(&entry,sizeof(entry),1,file);fclose(file);
 NSDictionary *meta=LegacySaveMetadata(legacyPath);assert([meta[@"title"] isEqual:@"Legacy Test"]);assert([meta[@"level"] intValue]==7);
 [@"invalid" writeToFile:legacyPath atomically:YES encoding:NSUTF8StringEncoding error:NULL];assert(LegacySaveMetadata(legacyPath).count==0);
 puts("PASS: remapping preserves keyboard modifiers and unrelated settings; new actions are inserted into the keys section");
}return 0;}
