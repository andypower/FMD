  function GetWPMangaInfoFromURL: Byte;
  var
    s: String;
    i, j: Integer;
    isSummaryDone: Boolean = False;
    isInfo: Boolean = False;
    isInvertChapters: Boolean = True;
    pnumber: Integer = 1;

    procedure FindChapters;
    var
      x, y: Integer;
      StartFindPage: Boolean = False;
    begin
      if parse.Count > 0 then
        for x := 0 to parse.Count-1 do
        begin
          s := LowerCase(GetVal(parse[x], 'class'));
          if (GetTagName(parse[x]) = 'ul') and
            (AnsiIndexText(s, ['lst', 'chp_lst']) > -1) then
          begin
            StartFindPage := True;
            if s = 'chp_lst' then
              isInvertChapters := False;
          end;
          if StartFindPage then
          begin
            if GetTagName(parse[x]) = '/ul' then
              Break
            else
            if GetTagName(parse[x]) = 'a' then
            begin
              if Pos('class="c4', parse[x]) = 0 then
              begin
                mangaInfo.chapterLinks.Add(GetVal(parse[x], 'href'));
                s := '';
                for y := x+1 to parse.Count-1 do
                begin
                  s := Trim(parse[y]);
                  if (s <> '') and (Pos('<', s) <> 1) then
                    Break;
                end;
                mangaInfo.chapterName.Add(CommonStringFilter(s));
              end;
            end;
          end;
        end;
    end;

  begin
    mangaInfo.website := WebsiteRoots[WebsiteID, 0];
    mangaInfo.url := FillMangaSiteHost(WebsiteID, URL);

    if RightStr(mangaInfo.url, 1) <> '/' then
      mangaInfo.url += '/';

    if not GetPage(TObject(Source), mangaInfo.url, Reconnect) then
    begin
      Result := NET_PROBLEM;
      Source.Free;
      Exit;
    end;

    ParseHTML(Source.Text, parse);
    Source.Free;

    if parse.Count > 0 then
    begin
      mangaInfo.genres := '';
      mangaInfo.summary := '';
      for i := 0 to parse.Count-1 do
      begin
        //cover
        if mangaInfo.coverLink = '' then
        begin
          if (Pos('<img ', parse[i]) <> 0) and
            (Pos('class="cvr', parse[i]) <> 0) then
            mangaInfo.coverLink := GetVal(parse[i], 'src');
        end;

        //title
        if mangaInfo.title = '' then
        begin
          if (Pos('class="ttl"', parse[i]) <> 0) or
            (Pos('itemprop="itemreviewed"', parse[i]) <> 0)
          then
            mangaInfo.title := CommonStringFilter(parse[i+1])
        end;

        //details
        s := GetVal(parse[i], 'class');
        if (GetTagName(parse[i]) = 'div') and
          (AnsiIndexText(s, ['det', 'lts_chp fr', 'mng_ifo']) > -1) then
          isInfo := True;
        if isInfo then
        begin
          if GetTagName(parse[i]) = 'h2' then
            isInfo := False
          else
          begin
            s := Trim(parse[i]);
            //author
            if s = 'Author' then
              mangaInfo.authors := CommonStringFilter(
                TrimLeftChar(parse[i+2], [':', ' ']))
            else
            if Pos('/author/', s) <> 0 then
              mangaInfo.authors := CommonStringFilter(parse[i+1])
            //artist
            else
            if s = 'Artist' then
              mangaInfo.artists := CommonStringFilter(
                TrimLeftChar(parse[i+2], [':', ' ']))
            //status
            else
            if s = 'Status' then
            begin
              if Pos('completed', LowerCase(parse[i+2])) <> 0 then
                mangaInfo.status := '0'
              else
                mangaInfo.status := '1';
            end
            //genres
            else
            if (s = 'Category') or (s = 'Genres') then
            begin
              for j := i+3 to parse.Count-1 do
              begin
                if GetTagName(parse[j]) = '/p' then
                  Break
                else
                if Pos('<', parse[j]) = 0 then
                  AddCommaString(mangaInfo.genres, CommonStringFilter(parse[j]));
              end;
            end
            else
            if s = 'Type' then
              AddCommaString(mangaInfo.genres,
                CommonStringFilter(TrimLeftChar(parse[i+2], [':', ' '])))
            else
            //summary
            if (not isSummaryDone) and (s = 'Summary') then
            begin
              isSummaryDone := True;
              for j := i+2 to parse.Count-1 do
              begin
                if GetTagName(parse[j]) = '/p' then
                  Break
                else
                if Pos('<', parse[j]) = 0 then
                  mangaInfo.summary := Trim(mangaInfo.summary + LineEnding +
                    CommonStringFilter(parse[j]));
              end;
            end;
            //summary
            if (not isSummaryDone) and (GetTagName(parse[i]) = 'p') then
            begin
              s := Trim(parse[i+1]);
              if (s <> '') and (Pos('<', s) = 0) then
              begin
                isSummaryDone := True;
                for j := i+1 to parse.Count-1 do
                begin
                  if GetTagName(parse[j]) = '/p' then
                    Break
                  else
                  if Pos('<', parse[j]) = 0 then
                    mangaInfo.summary := Trim(mangaInfo.summary + LineEnding +
                      CommonStringFilter(parse[j]));
                end;
              end;
            end;
          end;
        end;
      end;

      //chapters
      FindChapters;
      for i := parse.Count-1 downto 0 do
      begin
        if (Pos('<a', parse[i]) <> 0) and
          (Pos('/chapter-list/', parse[i]) <> 0) then
        begin
          s := GetVal(parse[i], 'href');
          pnumber := StrToIntDef(
            ReplaceRegExpr('^.*/chapter-list/(\d+)/$', s, '$1', True), 1);
          Break;
        end;
      end;
      if pnumber > 1 then
      begin
        Source := TStringList.Create;
        for i := 2 to pnumber do
        begin
          if GetPage(TObject(Source),
            mangaInfo.url+IntToStr(i)+'/', Reconnect) then
          begin
            ParseHTML(Source.Text, parse);
            FindChapters;
          end;
        end;
        Source.Free;
      end;

      //invert chapters
      if isInvertChapters then
        InvertStrings([mangaInfo.chapterName, mangaInfo.chapterLinks]);
      Result := NO_ERROR;
    end;
  end;