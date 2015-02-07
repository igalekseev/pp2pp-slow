<?xml version='1.0'?>
<Project Type="Project" LVVersion="8208000">
   <Item Name="My Computer" Type="My Computer">
      <Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
      <Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
      <Property Name="server.tcp.enabled" Type="Bool">false</Property>
      <Property Name="server.tcp.port" Type="Int">0</Property>
      <Property Name="server.tcp.serviceName" Type="Str">My Computer/VI Server</Property>
      <Property Name="server.tcp.serviceName.default" Type="Str">My Computer/VI Server</Property>
      <Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
      <Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
      <Property Name="specify.custom.address" Type="Bool">false</Property>
      <Item Name="lv-pp2pp-slow.vi" Type="VI" URL="lv-pp2pp-slow.vi"/>
      <Item Name="manageLV.vi" Type="VI" URL="manageLV.vi"/>
      <Item Name="checkExp.vi" Type="VI" URL="checkExp.vi"/>
      <Item Name="lv-pp2pp-slow.so" Type="Document" URL="../lv-pp2pp-slow.so"/>
      <Item Name="OneButton.vi" Type="VI" URL="OneButton.vi"/>
      <Item Name="Dependencies" Type="Dependencies"/>
      <Item Name="Build Specifications" Type="Build">
         <Item Name="lv-ppslow" Type="EXE">
            <Property Name="Absolute[0]" Type="Bool">false</Property>
            <Property Name="Absolute[1]" Type="Bool">false</Property>
            <Property Name="Absolute[2]" Type="Bool">false</Property>
            <Property Name="ActiveXInfoEnumCLSIDsItemCount" Type="Int">0</Property>
            <Property Name="ActiveXInfoMajorVersion" Type="Int">0</Property>
            <Property Name="ActiveXInfoMinorVersion" Type="Int">0</Property>
            <Property Name="ActiveXInfoObjCLSIDsItemCount" Type="Int">0</Property>
            <Property Name="ActiveXInfoProgIDPrefix" Type="Str"></Property>
            <Property Name="ActiveXServerName" Type="Str"></Property>
            <Property Name="AliasID" Type="Str">{D810335B-AC18-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="AliasName" Type="Str">Project.aliases</Property>
            <Property Name="ApplicationID" Type="Str">{D80FD663-AC18-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="ApplicationName" Type="Str">lv-ppslow</Property>
            <Property Name="AutoIncrement" Type="Bool">false</Property>
            <Property Name="BuildName" Type="Str">lv-ppslow</Property>
            <Property Name="CommandLineArguments" Type="Bool">false</Property>
            <Property Name="CopyErrors" Type="Bool">false</Property>
            <Property Name="DebuggingEXE" Type="Bool">false</Property>
            <Property Name="DebugServerWaitOnLaunch" Type="Bool">false</Property>
            <Property Name="DefaultLanguage" Type="Str">English</Property>
            <Property Name="DependencyApplyDestination" Type="Bool">true</Property>
            <Property Name="DependencyApplyInclusion" Type="Bool">true</Property>
            <Property Name="DependencyApplyProperties" Type="Bool">true</Property>
            <Property Name="DependencyFolderDestination" Type="Int">0</Property>
            <Property Name="DependencyFolderInclusion" Type="Str">As needed</Property>
            <Property Name="DependencyFolderPropertiesItemCount" Type="Int">0</Property>
            <Property Name="DestinationID[0]" Type="Str">{D8119D59-AC18-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="DestinationID[1]" Type="Str">{D8119D59-AC18-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="DestinationID[2]" Type="Str">{D8119313-AC18-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="DestinationItemCount" Type="Int">3</Property>
            <Property Name="DestinationName[0]" Type="Str">lv-ppslow</Property>
            <Property Name="DestinationName[1]" Type="Str">Destination Directory</Property>
            <Property Name="DestinationName[2]" Type="Str">Support Directory</Property>
            <Property Name="Disconnect" Type="Bool">true</Property>
            <Property Name="IncludeHWConfig" Type="Bool">false</Property>
            <Property Name="IncludeSCC" Type="Bool">true</Property>
            <Property Name="INIID" Type="Str">{D8103A31-AC18-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="ININame" Type="Str">LabVIEW.ini</Property>
            <Property Name="LOGID" Type="Str">{D8104071-AC18-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="MathScript" Type="Bool">false</Property>
            <Property Name="Path[0]" Type="Path">../builds/lv-ppslow/internal.llb</Property>
            <Property Name="Path[1]" Type="Path">../builds/lv-ppslow</Property>
            <Property Name="Path[2]" Type="Path">../builds/lv-ppslow/data</Property>
            <Property Name="ShowHWConfig" Type="Bool">false</Property>
            <Property Name="SourceInfoItemCount" Type="Int">5</Property>
            <Property Name="SourceItem[0].Inclusion" Type="Str">Startup VI</Property>
            <Property Name="SourceItem[0].ItemID" Type="Ref">/My Computer/lv-pp2pp-slow.vi</Property>
            <Property Name="SourceItem[0].VIPropertiesItemCount" Type="Int">7</Property>
            <Property Name="SourceItem[0].VIPropertiesSettingOption[0]" Type="Str">Remove panel</Property>
            <Property Name="SourceItem[0].VIPropertiesSettingOption[1]" Type="Str">Run when opened</Property>
            <Property Name="SourceItem[0].VIPropertiesSettingOption[2]" Type="Str">Show abort button</Property>
            <Property Name="SourceItem[0].VIPropertiesSettingOption[3]" Type="Str">Allow user to close window</Property>
            <Property Name="SourceItem[0].VIPropertiesSettingOption[4]" Type="Str">Show menu bar</Property>
            <Property Name="SourceItem[0].VIPropertiesSettingOption[5]" Type="Str">Show tool bar when running</Property>
            <Property Name="SourceItem[0].VIPropertiesSettingOption[6]" Type="Str">Show scroll bars</Property>
            <Property Name="SourceItem[0].VIPropertiesVISetting[0]" Type="Bool">false</Property>
            <Property Name="SourceItem[0].VIPropertiesVISetting[1]" Type="Bool">true</Property>
            <Property Name="SourceItem[0].VIPropertiesVISetting[2]" Type="Bool">false</Property>
            <Property Name="SourceItem[0].VIPropertiesVISetting[3]" Type="Bool">false</Property>
            <Property Name="SourceItem[0].VIPropertiesVISetting[4]" Type="Bool">false</Property>
            <Property Name="SourceItem[0].VIPropertiesVISetting[5]" Type="Bool">false</Property>
            <Property Name="SourceItem[0].VIPropertiesVISetting[6]" Type="Bool">false</Property>
            <Property Name="SourceItem[1].Inclusion" Type="Str">Always Included</Property>
            <Property Name="SourceItem[1].ItemID" Type="Ref">/My Computer/manageLV.vi</Property>
            <Property Name="SourceItem[1].VIPropertiesItemCount" Type="Int">1</Property>
            <Property Name="SourceItem[1].VIPropertiesSettingOption[0]" Type="Str">Remove panel</Property>
            <Property Name="SourceItem[1].VIPropertiesVISetting[0]" Type="Bool">false</Property>
            <Property Name="SourceItem[2].Inclusion" Type="Str">Always Included</Property>
            <Property Name="SourceItem[2].ItemID" Type="Ref">/My Computer/checkExp.vi</Property>
            <Property Name="SourceItem[2].VIPropertiesItemCount" Type="Int">1</Property>
            <Property Name="SourceItem[2].VIPropertiesSettingOption[0]" Type="Str">Remove panel</Property>
            <Property Name="SourceItem[2].VIPropertiesVISetting[0]" Type="Bool">false</Property>
            <Property Name="SourceItem[3].Destination" Type="Int">1</Property>
            <Property Name="SourceItem[3].Inclusion" Type="Str">Always Included</Property>
            <Property Name="SourceItem[3].ItemID" Type="Ref">/My Computer/lv-pp2pp-slow.so</Property>
            <Property Name="SourceItem[4].ItemID" Type="Ref">/My Computer/OneButton.vi</Property>
            <Property Name="SourceItem[4].VIPropertiesItemCount" Type="Int">1</Property>
            <Property Name="SourceItem[4].VIPropertiesSettingOption[0]" Type="Str">Remove panel</Property>
            <Property Name="SourceItem[4].VIPropertiesVISetting[0]" Type="Bool">false</Property>
            <Property Name="StripLib" Type="Bool">true</Property>
            <Property Name="SupportedLanguageCount" Type="Int">0</Property>
            <Property Name="TLBID" Type="Str">{D8103D8D-AC18-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="UseFFRTE" Type="Bool">false</Property>
            <Property Name="VersionInfoCompanyName" Type="Str"></Property>
            <Property Name="VersionInfoFileDescription" Type="Str"></Property>
            <Property Name="VersionInfoFileType" Type="Int">1</Property>
            <Property Name="VersionInfoFileVersionBuild" Type="Int">0</Property>
            <Property Name="VersionInfoFileVersionMajor" Type="Int">1</Property>
            <Property Name="VersionInfoFileVersionMinor" Type="Int">0</Property>
            <Property Name="VersionInfoFileVersionPatch" Type="Int">0</Property>
            <Property Name="VersionInfoInternalName" Type="Str">My Application</Property>
            <Property Name="VersionInfoLegalCopyright" Type="Str">Copyright © 2015 </Property>
            <Property Name="VersionInfoProductName" Type="Str">My Application</Property>
         </Item>
         <Item Name="oneButton" Type="EXE">
            <Property Name="Absolute[0]" Type="Bool">false</Property>
            <Property Name="Absolute[1]" Type="Bool">false</Property>
            <Property Name="Absolute[2]" Type="Bool">false</Property>
            <Property Name="ActiveXServerName" Type="Str"></Property>
            <Property Name="AliasID" Type="Str">{BE23ACC5-AE70-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="AliasName" Type="Str">Project.aliases</Property>
            <Property Name="ApplicationID" Type="Str">{BE232485-AE70-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="ApplicationName" Type="Str">oneButton</Property>
            <Property Name="AutoIncrement" Type="Bool">false</Property>
            <Property Name="BuildName" Type="Str">oneButton</Property>
            <Property Name="CommandLineArguments" Type="Bool">false</Property>
            <Property Name="CopyErrors" Type="Bool">false</Property>
            <Property Name="DebuggingEXE" Type="Bool">false</Property>
            <Property Name="DebugServerWaitOnLaunch" Type="Bool">false</Property>
            <Property Name="DefaultLanguage" Type="Str">English</Property>
            <Property Name="DependencyApplyDestination" Type="Bool">true</Property>
            <Property Name="DependencyApplyInclusion" Type="Bool">true</Property>
            <Property Name="DependencyApplyProperties" Type="Bool">true</Property>
            <Property Name="DependencyFolderDestination" Type="Int">0</Property>
            <Property Name="DependencyFolderInclusion" Type="Str">As needed</Property>
            <Property Name="DependencyFolderPropertiesItemCount" Type="Int">0</Property>
            <Property Name="DestinationID[0]" Type="Str">{BE24E8B5-AE70-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="DestinationID[1]" Type="Str">{BE24E8B5-AE70-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="DestinationID[2]" Type="Str">{BE24DE3D-AE70-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="DestinationItemCount" Type="Int">3</Property>
            <Property Name="DestinationName[0]" Type="Str">oneButton</Property>
            <Property Name="DestinationName[1]" Type="Str">Destination Directory</Property>
            <Property Name="DestinationName[2]" Type="Str">Support Directory</Property>
            <Property Name="Disconnect" Type="Bool">true</Property>
            <Property Name="IncludeHWConfig" Type="Bool">false</Property>
            <Property Name="IncludeSCC" Type="Bool">true</Property>
            <Property Name="INIID" Type="Str">{BE23B445-AE70-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="ININame" Type="Str">LabVIEW.ini</Property>
            <Property Name="LOGID" Type="Str">{BE23BAD5-AE70-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="MathScript" Type="Bool">false</Property>
            <Property Name="Path[0]" Type="Path">../builds/onebutton/internal.llb</Property>
            <Property Name="Path[1]" Type="Path">../builds/onebutton</Property>
            <Property Name="Path[2]" Type="Path">../builds/onebutton/data</Property>
            <Property Name="ShowHWConfig" Type="Bool">false</Property>
            <Property Name="SourceInfoItemCount" Type="Int">5</Property>
            <Property Name="SourceItem[0].ItemID" Type="Ref">/My Computer/lv-pp2pp-slow.vi</Property>
            <Property Name="SourceItem[0].VIPropertiesItemCount" Type="Int">1</Property>
            <Property Name="SourceItem[0].VIPropertiesSettingOption[0]" Type="Str">Remove panel</Property>
            <Property Name="SourceItem[0].VIPropertiesVISetting[0]" Type="Bool">false</Property>
            <Property Name="SourceItem[1].ItemID" Type="Ref">/My Computer/manageLV.vi</Property>
            <Property Name="SourceItem[1].VIPropertiesItemCount" Type="Int">1</Property>
            <Property Name="SourceItem[1].VIPropertiesSettingOption[0]" Type="Str">Remove panel</Property>
            <Property Name="SourceItem[1].VIPropertiesVISetting[0]" Type="Bool">false</Property>
            <Property Name="SourceItem[2].ItemID" Type="Ref">/My Computer/checkExp.vi</Property>
            <Property Name="SourceItem[2].VIPropertiesItemCount" Type="Int">1</Property>
            <Property Name="SourceItem[2].VIPropertiesSettingOption[0]" Type="Str">Remove panel</Property>
            <Property Name="SourceItem[2].VIPropertiesVISetting[0]" Type="Bool">false</Property>
            <Property Name="SourceItem[3].Destination" Type="Int">1</Property>
            <Property Name="SourceItem[3].Inclusion" Type="Str">Always Included</Property>
            <Property Name="SourceItem[3].ItemID" Type="Ref">/My Computer/lv-pp2pp-slow.so</Property>
            <Property Name="SourceItem[4].Inclusion" Type="Str">Startup VI</Property>
            <Property Name="SourceItem[4].ItemID" Type="Ref">/My Computer/OneButton.vi</Property>
            <Property Name="SourceItem[4].VIPropertiesItemCount" Type="Int">2</Property>
            <Property Name="SourceItem[4].VIPropertiesSettingOption[0]" Type="Str">Remove panel</Property>
            <Property Name="SourceItem[4].VIPropertiesSettingOption[1]" Type="Str">Run when opened</Property>
            <Property Name="SourceItem[4].VIPropertiesVISetting[0]" Type="Bool">false</Property>
            <Property Name="SourceItem[4].VIPropertiesVISetting[1]" Type="Bool">true</Property>
            <Property Name="StripLib" Type="Bool">true</Property>
            <Property Name="SupportedLanguageCount" Type="Int">0</Property>
            <Property Name="TLBID" Type="Str">{BE23B7F1-AE70-11E4-A7C6-00065B01ED8F}</Property>
            <Property Name="UseFFRTE" Type="Bool">false</Property>
            <Property Name="VersionInfoCompanyName" Type="Str"></Property>
            <Property Name="VersionInfoFileDescription" Type="Str"></Property>
            <Property Name="VersionInfoFileType" Type="Int">1</Property>
            <Property Name="VersionInfoFileVersionBuild" Type="Int">0</Property>
            <Property Name="VersionInfoFileVersionMajor" Type="Int">1</Property>
            <Property Name="VersionInfoFileVersionMinor" Type="Int">0</Property>
            <Property Name="VersionInfoFileVersionPatch" Type="Int">0</Property>
            <Property Name="VersionInfoInternalName" Type="Str">My Application</Property>
            <Property Name="VersionInfoLegalCopyright" Type="Str">Copyright © 2015 </Property>
            <Property Name="VersionInfoProductName" Type="Str">My Application</Property>
         </Item>
      </Item>
   </Item>
</Project>
