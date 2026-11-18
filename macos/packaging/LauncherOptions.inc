- (void)compareVersion:(NSString *)version {
    NSAlert *compare=[NSAlert new];compare.messageText=@"Compare your friend's game version";compare.informativeText=@"Enter the four-part version shown by the other player's diagnostics.";
    NSTextField *field=[[NSTextField alloc]initWithFrame:NSMakeRect(0,0,260,24)];field.placeholderString=@"1.4.0.0";compare.accessoryView=field;[compare addButtonWithTitle:@"Compare"];[compare addButtonWithTitle:@"Cancel"];
    if([compare runModal]==NSAlertFirstButtonReturn){
        NSString *peer=[field.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
        NSString *message=[peer isEqual:version]?@"Game version numbers match. Also check the Mac app build and selected mods; a full connection test is still required.":@"Game versions differ. Update both players to the same version before hosting or joining.";
        dispatch_async(dispatch_get_main_queue(),^{[self alert:message];});
    }
}
- (void)latestSave:(id)sender {
    (void)sender;if(self.working||self.gamePID>0)return;
    NSString *folder=[Prefs() stringByAppendingPathComponent:@"save"];NSDate *latest=nil;NSNumber *slot=nil;
    NSRegularExpression *rx=[NSRegularExpression regularExpressionWithPattern:@"^fx1g([0-9]{4})\\.sav$" options:0 error:nil];
    for(NSString *file in [FM() contentsOfDirectoryAtPath:folder error:nil]){
        NSTextCheckingResult *m=[rx firstMatchInString:file options:0 range:NSMakeRange(0,file.length)];if(!m)continue;
        NSDictionary *a=[FM() attributesOfItemAtPath:[folder stringByAppendingPathComponent:file] error:nil];
        if([a[NSFileSize] unsignedLongLongValue]<24)continue;
        NSDate *date=a[NSFileModificationDate];if(!latest || [date compare:latest]==NSOrderedDescending){latest=date;slot=@([[file substringWithRange:[m rangeAtIndex:1]] integerValue]);}
    }
    if(!slot){[self alert:@"No saved games are available yet."];return;}
    self.pendingLoadSlot=slot;[self playGame:nil];
}
- (void)options:(id)sender {
    (void)sender;if(self.working||self.gamePID>0)return;
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;
    NSAlert *a=[NSAlert new];a.messageText=@"Game options";a.informativeText=@"Original keeps crisp pixels. Enhanced smooths scaled presentation and uses compatible imported MP4 movies when available. Missing or invalid replacements fall back to the original movies. Autosaves apply only to single-player games.";
    NSStackView *stack=[NSStackView new];stack.frame=NSMakeRect(0,0,540,350);stack.orientation=NSUserInterfaceLayoutOrientationVertical;stack.alignment=NSLayoutAttributeLeading;stack.spacing=10;
    NSPopUpButton *presentation=[NSPopUpButton new];[presentation addItemsWithTitles:@[@"Original",@"Enhanced"]];[presentation selectItemAtIndex:[d boolForKey:@"enhanced"]?1:0];
    NSPopUpButton *autosave=[NSPopUpButton new];[autosave addItemsWithTitles:@[@"Off",@"Every 2 minutes",@"Every 5 minutes",@"Every 10 minutes"]];NSArray *seconds=@[@0,@120,@300,@600];NSUInteger index=[seconds indexOfObject:[d objectForKey:@"autosaveSeconds"]?:@300];[autosave selectItemAtIndex:index==NSNotFound?2:index];
    NSSlider *speed=[NSSlider sliderWithValue:[d objectForKey:@"cursorSpeed"]?[d doubleForKey:@"cursorSpeed"]:1 minValue:0.25 maxValue:3 target:nil action:nil];[speed.widthAnchor constraintEqualToConstant:260].active=YES;
    NSPopUpButton *confirm=[NSPopUpButton new],*back=[NSPopUpButton new];NSArray *faces=@[@"A / Cross",@"B / Circle",@"X / Square",@"Y / Triangle"];
    [confirm addItemsWithTitles:faces];[back addItemsWithTitles:faces];[confirm selectItemAtIndex:[d integerForKey:@"menuConfirm"]];[back selectItemAtIndex:[d objectForKey:@"menuBack"]?[d integerForKey:@"menuBack"]:1];
    NSSlider *deadzone=[NSSlider sliderWithValue:[d objectForKey:@"controllerDeadzone"]?[d doubleForKey:@"controllerDeadzone"]:8000.0/32767 minValue:0.05 maxValue:0.5 target:nil action:nil];
    NSSlider *rumble=[NSSlider sliderWithValue:[d objectForKey:@"controllerRumble"]?[d doubleForKey:@"controllerRumble"]:1 minValue:0 maxValue:1 target:nil action:nil];
    for(NSSlider *slider in @[deadzone,rumble]) [slider.widthAnchor constraintEqualToConstant:260].active=YES;
    NSPopUpButton *performance=[NSPopUpButton new];[performance addItemsWithTitles:@[@"Balanced — 60 FPS",@"Battery saver — 30 FPS",@"High refresh — 120 FPS",@"Unlimited"]];[performance selectItemAtIndex:MAX(0,MIN(3,[d integerForKey:@"performancePreset"]))];
    NSButton *overlay=[NSButton checkboxWithTitle:@"Show frame timings" target:nil action:nil];overlay.state=[d boolForKey:@"performanceOverlay"];
    for(NSArray *row in @[@[@"Performance",performance],@[@"Performance overlay",overlay],@[@"Stick dead zone (5–50%)",deadzone],@[@"Vibration strength (off–full)",rumble],@[@"Presentation",presentation],@[@"Rotating autosaves",autosave],@[@"Controller pointer speed",speed],@[@"Menu confirm",confirm],@[@"Menu back",back]]){
        NSStackView *line=[NSStackView stackViewWithViews:@[[NSTextField labelWithString:row[0]],row[1]]];line.spacing=14;[stack addArrangedSubview:line];
    }
    a.accessoryView=stack;[a addButtonWithTitle:@"Save"];[a addButtonWithTitle:@"Cancel"];[a addButtonWithTitle:@"Import Enhanced Movies…"];[a addButtonWithTitle:@"Restore Save Backup…"];
    NSModalResponse response=[a runModal];
    if(response==NSAlertFirstButtonReturn){
        if(confirm.indexOfSelectedItem==back.indexOfSelectedItem){[self alert:@"Choose different buttons for confirm and back."];[self options:nil];return;}
        [d setBool:presentation.indexOfSelectedItem==1 forKey:@"enhanced"];[d setObject:seconds[autosave.indexOfSelectedItem] forKey:@"autosaveSeconds"];[d setDouble:speed.doubleValue forKey:@"cursorSpeed"];[d setInteger:confirm.indexOfSelectedItem forKey:@"menuConfirm"];[d setInteger:back.indexOfSelectedItem forKey:@"menuBack"];
        [d setDouble:deadzone.doubleValue forKey:@"controllerDeadzone"];[d setDouble:rumble.doubleValue forKey:@"controllerRumble"];[d setInteger:performance.indexOfSelectedItem forKey:@"performancePreset"];[d setBool:overlay.state==NSControlStateValueOn forKey:@"performanceOverlay"];
        self.status.stringValue=@"Options saved. Changes apply the next time you play.";
    }else if(response==NSAlertThirdButtonReturn)dispatch_async(dispatch_get_main_queue(),^{[self importEnhanced:nil];});
    else if(response==NSAlertFirstButtonReturn+3)dispatch_async(dispatch_get_main_queue(),^{[self restoreBackup:nil];});
}
- (void)bindingActionChanged:(id)sender {
    (void)sender;NSArray *actions=ControllerActions();NSInteger i=self.bindingAction.indexOfSelectedItem;if(i<0||(NSUInteger)i>=actions.count)return;
    NSString *key=actions[i][@"action"];
    self.bindingValue.stringValue=[NSUserDefaults.standardUserDefaults dictionaryForKey:@"controllerBindings"][key]?:actions[i][@"default"];
}
- (void)remapControls:(id)sender {
    (void)sender;if(self.working||self.gamePID>0)return;NSArray *actions=ControllerActions();if(!actions.count){[self alert:@"Controller action definitions are unavailable."];return;}
    NSAlert *a=[NSAlert new];a.messageText=@"Remap controller actions";a.informativeText=@"Choose an action and a button or stick direction. Use | to combine buttons, for example A|DPAD_UP. Save Binding changes only the selected action and preserves its keyboard binding. Menu confirm/back and pointer speed are in Options.";
    self.bindingAction=[NSPopUpButton new];for(NSDictionary *entry in actions)[self.bindingAction addItemWithTitle:entry[@"action"]];self.bindingAction.target=self;self.bindingAction.action=@selector(bindingActionChanged:);
    self.bindingValue=[[NSComboBox alloc]initWithFrame:NSMakeRect(0,0,380,26)];[self.bindingValue addItemsWithObjectValues:@[@"NONE",@"A",@"B",@"X",@"Y",@"START",@"BACK",@"LEFTSHOULDER",@"RIGHTSHOULDER",@"LEFTSTICK",@"RIGHTSTICK",@"L2",@"R2",@"DPAD_UP",@"DPAD_DOWN",@"DPAD_LEFT",@"DPAD_RIGHT",@"LS_UP",@"LS_DOWN",@"LS_LEFT",@"LS_RIGHT",@"RS_UP",@"RS_DOWN",@"RS_LEFT",@"RS_RIGHT",@"TOUCHPAD",@"A|DPAD_UP",@"A|DPAD_DOWN",@"A|DPAD_LEFT",@"A|DPAD_RIGHT"]];
    self.guideModel=[NSPopUpButton new];[self.guideModel addItemsWithTitles:@[@"Xbox",@"DualSense / PlayStation"]];self.guideModel.target=self;self.guideModel.action=@selector(guideModelChanged:);
    self.guideImage=[[NSImageView alloc]initWithFrame:NSMakeRect(0,0,600,420)];self.guideImage.imageScaling=NSImageScaleProportionallyUpOrDown;[self.guideImage.widthAnchor constraintEqualToConstant:600].active=YES;[self.guideImage.heightAnchor constraintEqualToConstant:420].active=YES;
    [self guideModelChanged:nil];
    NSStackView *stack=[NSStackView stackViewWithViews:@[self.guideModel,self.guideImage,[NSTextField labelWithString:@"Diagram shows defaults. Select an action below to view or change its binding."],self.bindingAction,self.bindingValue]];stack.orientation=NSUserInterfaceLayoutOrientationVertical;stack.frame=NSMakeRect(0,0,620,560);a.accessoryView=stack;[self bindingActionChanged:nil];
    [a addButtonWithTitle:@"Save Binding"];[a addButtonWithTitle:@"Done"];[a addButtonWithTitle:@"Reset All to Defaults"];
    NSSet *allowed=[NSSet setWithArray:@[@"NONE",@"A",@"B",@"X",@"Y",@"START",@"BACK",@"LEFTSHOULDER",@"RIGHTSHOULDER",@"LEFTSTICK",@"RIGHTSTICK",@"L2",@"R2",@"DPAD_UP",@"DPAD_DOWN",@"DPAD_LEFT",@"DPAD_RIGHT",@"LS_UP",@"LS_DOWN",@"LS_LEFT",@"LS_RIGHT",@"RS_UP",@"RS_DOWN",@"RS_LEFT",@"RS_RIGHT",@"TOUCHPAD",@"MISC1",@"PADDLE1",@"PADDLE2",@"PADDLE3",@"PADDLE4"]];
    for(;;){NSModalResponse answer=[a runModal];if(answer==NSAlertSecondButtonReturn)break;
        NSMutableDictionary *bindings=[[NSUserDefaults.standardUserDefaults dictionaryForKey:@"controllerBindings"] mutableCopy]?:[NSMutableDictionary new];
        if(answer==NSAlertThirdButtonReturn){for(NSDictionary *entry in actions)bindings[entry[@"action"]]=entry[@"default"];[NSUserDefaults.standardUserDefaults setObject:bindings forKey:@"controllerBindings"];[self bindingActionChanged:nil];continue;}
        NSString *value=[[self.bindingValue.stringValue uppercaseString] stringByReplacingOccurrencesOfString:@" " withString:@""];BOOL valid=value.length>0;
        for(NSString *token in [value componentsSeparatedByString:@"|"])if(![allowed containsObject:token])valid=NO;
        if(!valid){[self alert:@"Choose a listed button or direction, or combine valid names using |."];continue;}
        bindings[actions[self.bindingAction.indexOfSelectedItem][@"action"]]=value;[NSUserDefaults.standardUserDefaults setObject:bindings forKey:@"controllerBindings"];
        self.status.stringValue=@"Controller binding saved; it applies on the next launch.";
    }
}
- (void)restoreBackup:(id)sender {
    (void)sender;if(self.working||self.gamePID>0)return;
    NSOpenPanel *panel=[NSOpenPanel openPanel];panel.directoryURL=[NSURL fileURLWithPath:[Prefs() stringByAppendingPathComponent:@"save"]];panel.message=@"Choose a .sav.backup1, .backup2 or .backup3 file. It will be restored into a new save slot; current saves remain unchanged.";
    if([panel runModal]!=NSModalResponseOK)return;
    NSString *name=panel.URL.lastPathComponent;NSRegularExpression *rx=[NSRegularExpression regularExpressionWithPattern:@"^fx1g[0-9]{4}\\.sav\\.backup[123]$" options:0 error:nil];
    if(![rx firstMatchInString:name options:0 range:NSMakeRange(0,name.length)]){[self alert:@"Select one of KeeperFX's numbered save backups."];return;}
    NSError *error=nil;NSString *folder=[Prefs() stringByAppendingPathComponent:@"save"];
    for(int slot=0;slot<10000;slot++){NSString *target=[folder stringByAppendingPathComponent:[NSString stringWithFormat:@"fx1g%04d.sav",slot]];if([FM() fileExistsAtPath:target])continue;
        if(CopyFile(panel.URL.path,target,&error)){[FM() setAttributes:@{NSFileModificationDate:NSDate.date} ofItemAtPath:target error:nil];self.status.stringValue=[NSString stringWithFormat:@"Backup restored into save slot %d. Open Latest Save to load it.",slot];}else[self alert:error.localizedDescription];return;
    }
    [self alert:@"No free save slot is available."];
}
- (void)importEnhanced:(id)sender {
    (void)sender;if(self.working||self.gamePID>0)return;
    NSOpenPanel *panel=[NSOpenPanel openPanel];panel.canChooseDirectories=YES;panel.canChooseFiles=NO;panel.message=@"Choose a folder containing compatible H.264/AAC MP4 replacements named intromix.mp4, outromix.mp4, bullfrog.mp4, ea.mp4 or drag.mp4.";
    if([panel runModal]!=NSModalResponseOK)return;
    NSString *source=panel.URL.path;self.working=YES;self.status.stringValue=@"Checking enhanced movies…";
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
        NSError *error=nil;NSMutableArray *files=[NSMutableArray new];NSString *failure=nil;
        for(NSString *name in @[@"intromix.mp4",@"outromix.mp4",@"bullfrog.mp4",@"ea.mp4",@"drag.mp4"]){NSString *path=Find(source,name);if(!path)continue;
            NSTask *task=[NSTask new];task.executableURL=[NSURL fileURLWithPath:[Resources() stringByAppendingPathComponent:@"video-check"]];task.arguments=@[path];NSPipe *pipe=[NSPipe pipe];task.standardOutput=pipe;task.standardError=pipe;
            if(![task launchAndReturnError:&error]){failure=error.localizedDescription;break;}[task waitUntilExit];
            NSString *result=[[NSString alloc]initWithData:pipe.fileHandleForReading.readDataToEndOfFile encoding:NSUTF8StringEncoding];if(task.terminationStatus!=0){failure=result.length?result:@"A replacement movie could not be decoded.";break;}[files addObject:@[path,name]];
        }
        if(!failure&&!files.count)failure=@"No supported replacement filenames were found.";
        NSString *stage=[Prefs() stringByAppendingPathComponent:[@"EnhancedImport-" stringByAppendingString:NSUUID.UUID.UUIDString]];
        if(!failure && [FM() fileExistsAtPath:[Prefs() stringByAppendingPathComponent:@"EnhancedData"]] && ![FM() copyItemAtPath:[Prefs() stringByAppendingPathComponent:@"EnhancedData"] toPath:stage error:&error])failure=error.localizedDescription;
        if(!failure){for(NSArray *file in files)if(!CopyFile(file[0],[stage stringByAppendingPathComponent:[@"enhanced/video" stringByAppendingPathComponent:file[1]]],&error)){failure=error.localizedDescription;break;}}
        if(!failure){NSString *dest=[Prefs() stringByAppendingPathComponent:@"EnhancedData"],*backup=[Prefs() stringByAppendingPathComponent:@"EnhancedData.previous"];[FM() removeItemAtPath:backup error:nil];BOOL previous=[FM() fileExistsAtPath:dest];
            if(previous&&![FM() moveItemAtPath:dest toPath:backup error:&error])failure=error.localizedDescription;
            if(!failure&&![FM() moveItemAtPath:stage toPath:dest error:&error]){failure=error.localizedDescription;if(previous)[FM() moveItemAtPath:backup toPath:dest error:nil];}
            if(!failure){[FM() removeItemAtPath:backup error:nil];[FM() removeItemAtPath:[Runtime() stringByAppendingPathComponent:@".asset-version"] error:nil];}
        }
        [FM() removeItemAtPath:stage error:nil];
        dispatch_async(dispatch_get_main_queue(),^{self.working=NO;self.status.stringValue=failure?:@"Enhanced movies imported. Choose Enhanced in Options to use them.";});
    });
}

- (void)guideModelChanged:(id)sender {
    (void)sender;NSString *name=self.guideModel.indexOfSelectedItem==1?@"controller-ps5.png":@"controller-xbox.png";
    self.guideImage.image=[[NSImage alloc]initWithContentsOfFile:[[Resources() stringByAppendingPathComponent:@"controllers"] stringByAppendingPathComponent:name]];
    self.guideImage.accessibilityLabel=self.guideModel.indexOfSelectedItem==1?@"DualSense default button guide":@"Xbox default button guide";
}
- (void)saveChoiceChanged:(id)sender {
    (void)sender;NSInteger index=self.saveChoice.indexOfSelectedItem;if(index<0 || (NSUInteger)index>=self.saveEntries.count)return;
    NSDictionary *entry=self.saveEntries[index],*meta=entry[@"meta"];NSString *path=entry[@"path"];
    self.saveImage.image=[[NSImage alloc]initWithContentsOfFile:[path stringByAppendingString:@".png"]];
    NSString *time=meta[@"levelSeconds"]?[NSString stringWithFormat:@"%ld min in this level",[meta[@"levelSeconds"] integerValue]/60]:@"Level time unavailable";
    self.saveDetails.stringValue=[NSString stringWithFormat:@"%@\n%@ • Level %@\n%@ • %@\n%@",meta[@"title"]?:[path lastPathComponent],meta[@"campaign"]?:@"Older save",meta[@"level"]?:@"?",entry[@"date"],time,self.saveImage.image?@"Preview captured when saved":@"No preview available for this save"];
}
- (void)browseSaves:(id)sender {
    (void)sender;if(self.working||self.gamePID>0)return;
    NSString *folder=[Prefs() stringByAppendingPathComponent:@"save"];NSMutableArray *entries=[NSMutableArray new];
    NSRegularExpression *rx=[NSRegularExpression regularExpressionWithPattern:@"^fx1g([0-9]{4})\\.sav(\\.backup[123])?$" options:0 error:nil];
    NSDateFormatter *dates=[NSDateFormatter new];dates.dateStyle=NSDateFormatterMediumStyle;dates.timeStyle=NSDateFormatterShortStyle;
    for(NSString *file in [FM() contentsOfDirectoryAtPath:folder error:nil]){
        NSTextCheckingResult *match=[rx firstMatchInString:file options:0 range:NSMakeRange(0,file.length)];if(!match)continue;
        NSString *path=[folder stringByAppendingPathComponent:file];NSDictionary *attrs=[FM() attributesOfItemAtPath:path error:nil];if([attrs[NSFileSize] unsignedLongLongValue]<24)continue;
        NSData *json=[NSData dataWithContentsOfFile:[path stringByAppendingString:@".json"]];id parsed=json?[NSJSONSerialization JSONObjectWithData:json options:0 error:nil]:nil;NSDictionary *meta=[parsed isKindOfClass:NSDictionary.class]?parsed:LegacySaveMetadata(path);
        NSDate *date=attrs[NSFileModificationDate]?:NSDate.distantPast;
        [entries addObject:@{@"path":path,@"slot":@([[file substringWithRange:[match rangeAtIndex:1]] integerValue]),@"backup":@([file containsString:@".backup"]),@"meta":meta,@"modified":date,@"date":[dates stringFromDate:date]}];
    }
    [entries sortUsingComparator:^NSComparisonResult(NSDictionary *a,NSDictionary *b){return [b[@"modified"] compare:a[@"modified"]];}];
    if(!entries.count){[self alert:@"No saved games are available yet."];return;}self.saveEntries=entries;
    self.saveChoice=[NSPopUpButton new];for(NSDictionary *entry in entries){NSDictionary *meta=entry[@"meta"];[self.saveChoice addItemWithTitle:[NSString stringWithFormat:@"Slot %@ — %@%@%@ — %@",entry[@"slot"],meta[@"title"]?:[entry[@"path"] lastPathComponent],[meta[@"automatic"] boolValue]?@" [Autosave]":@"",[entry[@"backup"] boolValue]?@" [Backup]":@"",entry[@"date"]]];}
    self.saveChoice.target=self;self.saveChoice.action=@selector(saveChoiceChanged:);
    self.saveImage=[[NSImageView alloc]initWithFrame:NSMakeRect(0,0,560,300)];self.saveImage.imageScaling=NSImageScaleProportionallyUpOrDown;[self.saveImage.widthAnchor constraintEqualToConstant:560].active=YES;[self.saveImage.heightAnchor constraintEqualToConstant:300].active=YES;
    self.saveDetails=[NSTextField wrappingLabelWithString:@""];[self.saveDetails.widthAnchor constraintEqualToConstant:560].active=YES;
    NSStackView *stack=[NSStackView stackViewWithViews:@[self.saveChoice,self.saveImage,self.saveDetails]];stack.orientation=NSUserInterfaceLayoutOrientationVertical;stack.frame=NSMakeRect(0,0,580,450);[self saveChoiceChanged:nil];
    NSAlert *a=[NSAlert new];a.messageText=@"Saved games";a.informativeText=@"Choose a save or one of its three backups. Loading a backup first restores it into a free slot.";a.accessoryView=stack;[a addButtonWithTitle:@"Load Selected"];[a addButtonWithTitle:@"Cancel"];[a addButtonWithTitle:@"Open Folder"];
    NSModalResponse answer=[a runModal];if(answer==NSAlertThirdButtonReturn){[self openSaves:nil];return;}if(answer!=NSAlertFirstButtonReturn)return;
    NSDictionary *entry=entries[self.saveChoice.indexOfSelectedItem];NSNumber *slot=entry[@"slot"];
    if([entry[@"backup"] boolValue]){
        slot=nil;NSError *error=nil;
        for(int i=0;i<10000;i++){NSString *target=[folder stringByAppendingPathComponent:[NSString stringWithFormat:@"fx1g%04d.sav",i]];if([FM() fileExistsAtPath:target])continue;
            if(!CopyFile(entry[@"path"],target,&error)){[self alert:error.localizedDescription];return;}
            for(NSString *suffix in @[@".json",@".png"]){NSString *source=[entry[@"path"] stringByAppendingString:suffix];if([FM() fileExistsAtPath:source])CopyFile(source,[target stringByAppendingString:suffix],nil);}
            [FM() setAttributes:@{NSFileModificationDate:NSDate.date} ofItemAtPath:target error:nil];slot=@(i);break;
        }
        if(!slot){[self alert:@"No free save slot is available."];return;}
    }
    self.pendingLoadSlot=slot;[self playGame:nil];
}
