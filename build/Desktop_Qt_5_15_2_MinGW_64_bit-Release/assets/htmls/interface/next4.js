
function checkForPol()
{
if(document.getElementById("idpm").checked==false &&  document.getElementById("idpf").checked==false 	)
{
alert("Сначала выберите пол пациента.");
return false;
}
return true;
}
function  selectPm()
{
document.getElementById("idm").style.display="inline-block";
document.getElementById("idw").style.display="none";		
}
function  selectPw()
{
document.getElementById("idm").style.display="none";
document.getElementById("idw").style.display="inline-block";	
}
function checkFone()
{
if(document.getElementById("idbl").checked==false &&  document.getElementById("idfl").checked==false 	)
{
alert("Вначале выберите параметр «Артериальное давление».");
return false;
}
return true;
}

function elChangex($elem)
{
 
if($elem.checked==false)
{ 
document.getElementById("idm").value="";
document.getElementById("idw").value="";	
document.getElementById("f1").value="";	
document.getElementById("f2").value="";	
document.getElementById("f3").value="";	
document.getElementById("f4").value="";	
document.getElementById("f5").value="";	
document.getElementById("idm").value="";	
document.getElementById("idw").value="";	

 
document.getElementById("idbls2").innerHTML="";
document.getElementById("idbls3").innerHTML="";
document.getElementById("idbls4").innerHTML="";
document.getElementById("idbls5").innerHTML="";
document.getElementById("idbls6").innerHTML="";
document.getElementById("idbls7").innerHTML="";
}
	}
function elChangex2($elem)
{
 
if($elem.checked==false)
{ 
document.getElementById("idm").value="";
document.getElementById("idw").value="";	
document.getElementById("f1").value="";	
document.getElementById("f2").value="";	
document.getElementById("f3").value="";	
document.getElementById("f4").value="";	
document.getElementById("f5").value="";	
document.getElementById("idvoz").value="";	

document.getElementById("idbls1").innerHTML="";
document.getElementById("idbls2").innerHTML="";
document.getElementById("idbls3").innerHTML="";
document.getElementById("idbls4").innerHTML="";
document.getElementById("idbls5").innerHTML="";
document.getElementById("idbls6").innerHTML="";
document.getElementById("idbls7").innerHTML="";
document.getElementById("idsum").innerHTML="";
document.getElementById("idv").innerHTML="<b>Вывод:</b>";
}
	}
function elChange($elem)
{
	 
	
	//сахарный диабет
	if($elem.id=="f1")
{
if(document.getElementById("idpm").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls3").innerHTML="2";
if($elem.value=="Нет")
document.getElementById("idbls3").innerHTML="0";
}
if(document.getElementById("idpf").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls3").innerHTML="3";
if($elem.value=="Нет")
document.getElementById("idbls3").innerHTML="0";
}
}
	//курение
	if($elem.id=="f2")
{
if(document.getElementById("idpm").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls4").innerHTML="3";
if($elem.value=="Нет")
document.getElementById("idbls4").innerHTML="0";
}
if(document.getElementById("idpf").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls4").innerHTML="3";
if($elem.value=="Нет")
document.getElementById("idbls4").innerHTML="0";
}
}
//ИБС, перемежающая хромота
	if($elem.id=="f3")
{
if(document.getElementById("idpm").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls5").innerHTML="3";
if($elem.value=="Нет")
document.getElementById("idbls5").innerHTML="0";
}
if(document.getElementById("idpf").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls5").innerHTML="2";
if($elem.value=="Нет")
document.getElementById("idbls5").innerHTML="0";
}
}
//Аритмия
	if($elem.id=="f4")
{
if(document.getElementById("idpm").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls6").innerHTML="4";
if($elem.value=="Нет")
document.getElementById("idbls6").innerHTML="0";
}
if(document.getElementById("idpf").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls6").innerHTML="6";
if($elem.value=="Нет")
document.getElementById("idbls6").innerHTML="0";
}
}
//Аритмия
	if($elem.id=="f4")
{
if(document.getElementById("idpm").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls6").innerHTML="4";
if($elem.value=="Нет")
document.getElementById("idbls6").innerHTML="0";
}

if(document.getElementById("idpf").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls6").innerHTML="6";
if($elem.value=="Нет")
document.getElementById("idbls6").innerHTML="0";
}
}
//Гипертрофия левого желудочка
	if($elem.id=="f5")
{
if(document.getElementById("idpm").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls7").innerHTML="6";
if($elem.value=="Нет")
document.getElementById("idbls7").innerHTML="0";
}

if(document.getElementById("idpf").checked==true )
{
if($elem.value=="Да")
document.getElementById("idbls7").innerHTML="4";
if($elem.value=="Нет")
document.getElementById("idbls7").innerHTML="0";
}
}

	if($elem.id=="idm")
{
 
if(document.getElementById("idbl").checked==true  && document.getElementById("idpm").checked==true )
{
if($elem.value=="95-105")
document.getElementById("idbls2").innerHTML="0";		
if($elem.value=="106-116")
document.getElementById("idbls2").innerHTML="1";	
if($elem.value=="117-126")
document.getElementById("idbls2").innerHTML="2";	
if($elem.value=="127-137")
document.getElementById("idbls2").innerHTML="3";	
if($elem.value=="138-148")
document.getElementById("idbls2").innerHTML="4";	
if($elem.value=="149-159")
document.getElementById("idbls2").innerHTML="5";	
if($elem.value=="160-170")
document.getElementById("idbls2").innerHTML="6";	
if($elem.value=="171-181")
document.getElementById("idbls2").innerHTML="7";
if($elem.value=="182-191")
document.getElementById("idbls2").innerHTML="8";
if($elem.value=="192-202")
document.getElementById("idbls2").innerHTML="9";
if($elem.value=="203-213")
document.getElementById("idbls2").innerHTML="10";		 	
}
if(document.getElementById("idfl").checked==true  && document.getElementById("idpm").checked==true )
{
if($elem.value=="95-105")
document.getElementById("idbls2").innerHTML="2";		
if($elem.value=="106-116")
document.getElementById("idbls2").innerHTML="3";	
if($elem.value=="117-126")
document.getElementById("idbls2").innerHTML="4";	
if($elem.value=="127-137")
document.getElementById("idbls2").innerHTML="5";	
if($elem.value=="138-148")
document.getElementById("idbls2").innerHTML="6";	
if($elem.value=="149-159")
document.getElementById("idbls2").innerHTML="7";	
if($elem.value=="160-170")
document.getElementById("idbls2").innerHTML="8";	
if($elem.value=="171-181")
document.getElementById("idbls2").innerHTML="9";
if($elem.value=="182-191")
document.getElementById("idbls2").innerHTML="10";
if($elem.value=="192-202")
document.getElementById("idbls2").innerHTML="11";
if($elem.value=="203-213")
document.getElementById("idbls2").innerHTML="12";		 	
}
}
	if($elem.id=="idw")
	{

if(document.getElementById("idbl").checked==true  && document.getElementById("idpf").checked==true )
{
 
   
	
 
if($elem.value=="95-104")
document.getElementById("idbls2").innerHTML="0";		
if($elem.value=="105-114")
document.getElementById("idbls2").innerHTML="1";	
if($elem.value=="115-124")
document.getElementById("idbls2").innerHTML="2";	
if($elem.value=="125-134")
document.getElementById("idbls2").innerHTML="3";	
if($elem.value=="135-144")
document.getElementById("idbls2").innerHTML="4";	
if($elem.value=="145-154")
document.getElementById("idbls2").innerHTML="5";	
if($elem.value=="155-164")
document.getElementById("idbls2").innerHTML="6";	
if($elem.value=="165-174")
document.getElementById("idbls2").innerHTML="7";
if($elem.value=="182-191")
document.getElementById("idbls2").innerHTML="8";
if($elem.value=="175-184")
document.getElementById("idbls2").innerHTML="9";
if($elem.value=="185-194")
document.getElementById("idbls2").innerHTML="10";		 	
}
if(document.getElementById("idfl").checked==true  && document.getElementById("idpf").checked==true )
{
	 
if($elem.value=="95-104")
document.getElementById("idbls2").innerHTML="6";		
if($elem.value=="105-114")
document.getElementById("idbls2").innerHTML="6";	
if($elem.value=="115-124")
document.getElementById("idbls2").innerHTML="7";	
if($elem.value=="125-134")
document.getElementById("idbls2").innerHTML="7";	
if($elem.value=="135-144")
document.getElementById("idbls2").innerHTML="7";	
if($elem.value=="145-154")
document.getElementById("idbls2").innerHTML="8";	
if($elem.value=="155-164")
document.getElementById("idbls2").innerHTML="8";	
if($elem.value=="165-174")
document.getElementById("idbls2").innerHTML="8";
if($elem.value=="182-191")
document.getElementById("idbls2").innerHTML="9";
if($elem.value=="175-184")
document.getElementById("idbls2").innerHTML="9";
if($elem.value=="185-194")
document.getElementById("idbls2").innerHTML="10";		 	
}
}
 
if($elem.id=="idvoz")
{
 
if($elem.value=="до 57")
{

document.getElementById("idbls1").innerHTML="0";	

}
if($elem.value=="57-59")
document.getElementById("idbls1").innerHTML="1";
if($elem.value=="60-62")
document.getElementById("idbls1").innerHTML="2";
if($elem.value=="63-65")
document.getElementById("idbls1").innerHTML="3";
if($elem.value=="66-68")
document.getElementById("idbls1").innerHTML="4";
if($elem.value=="69-71")
document.getElementById("idbls1").innerHTML="5";
if($elem.value=="72-74")
document.getElementById("idbls1").innerHTML="6";
if($elem.value=="75-77")
document.getElementById("idbls1").innerHTML="7";
if($elem.value=="78-80")
document.getElementById("idbls1").innerHTML="8";
if($elem.value=="81-83")
document.getElementById("idbls1").innerHTML="9";
if($elem.value=="84-86")
document.getElementById("idbls1").innerHTML="10";







	
}
}