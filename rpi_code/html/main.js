
function send_led_command(str)
{
    var url="cgi-bin/simple_cgi.sh?"+str;
    xmlHttp = GetXmlHttpObject();
    xmlHttp.onreadystatechange = xhr_state_changed;
    //alert("url " + url);
    xmlHttp.open("GET", url, true);
    xmlHttp.send(null);
}

function xhr_state_changed()
{

}

function GetXmlHttpObject()
{
    var x = null;
    try
    {
        // Firefox, Opera 8.0+, Safari
        x = new XMLHttpRequest();
    }

    catch (e)
    {
        // Internet Explorer
        try
        {
            x = new ActiveXObject("Msxml2.XMLHTTP");
        }
        catch (e)
        {
            x = new ActiveXObject("Microsoft.XMLHTTP");
        }
    }
    return x;
}

function body_onload()
{
    //alert("body_onload");
    // clear and update
    send_led_command("-c+-u");
}

function led_click(element, x, y)
{
    var hex_color = document.getElementById("colorpicker").value;
    var r, g, b;
    //alert("x:" + x + " y: " + y);

    element.style.background = hex_color;

    //TODO with the following always work?
    r = hex_color[1] + hex_color[2];
    g = hex_color[3] + hex_color[4];
    b = hex_color[5] + hex_color[6];

    send_led_command("-s+"+x+":"+y+":0x"+r+":0x"+g+":0x"+b+"+-u");
}


function colour_picked(element)
{
    //alert(element.value);
}


