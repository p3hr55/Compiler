var x,y,z:integer; 
      a,b,c:boolean;
procedure Average(p1,p2,p3:integer;var p4:integer);
var sum:integer;    
      done:boolean;
begin
    sum:=p1+p2+p3; //kk
    p4:=sum/3;
end

var w:boolean;

function abs(p1:integer):integer;
var local:integer;
begin    
    if p1  < 0 then    
    begin 
          abs:=p1*-1;
    end
    else    
    begin        
          abs:=p1;
    end;
end;

var e,f,g:boolean;    

begin

end.