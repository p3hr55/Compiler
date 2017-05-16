var a,b,c:integer;
procedure doit(x:integer; y:integer; var z:integer);
begin
    while x > 5 do
    begin
        z := z - 1;
        y := z * 2;
    end;
    
    if x = 0 then
        x := 1
    else
        x := 2
end;

function doit2(var x:integer):boolean;
begin
    while x < 0 do
    begin
        x := x - 1;
    end;
    
    doit2:=true;
end;

begin
doit(5, 6, c);
doit2(b);
end.