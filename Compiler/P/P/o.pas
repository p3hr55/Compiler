var i,j,temp:integer;
var k:array[1..10] of integer;
begin
       k[1]:=3;
       k[2]:=1;
       k[3]:=4;
       k[4]:=5;
       k[5]:=2;
       k[6]:=-7;
       k[7]:=8;
       k[8]:=99;
       k[9]:=0;
       k[10]:=5;
      
       i:=1;
 
       while i < 11 do
       begin
              j:=2;
              while j < 12 - i do
              begin
                     if k[j] < k[j-1] then
                     begin
                           temp:=k[j];
                           k[j]:=k[j-1];
                           k[j-1]:=temp;
                     end;
                     j:=j+1;
              end;
              i:=i+1;
       end;
end.

