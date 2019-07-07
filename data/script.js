var settings;

var settingsButton=document.getElementById("settings");
var osdSpan=document.getElementById("osd");
var line1Span=document.getElementById("line1");
var line2Span=document.getElementById("line2");
var osdDiv=document.getElementById("osddiv");
var modal = document.getElementById('myModal');
var closeModal = document.getElementsByClassName("close")[0];
var statusSpan=document.getElementById("status");
var saveButton = document.getElementById("save");
var rebootButton = document.getElementById("reboot");
var factoryresetButton = document.getElementById("factoryreset");

function checkKeycode(event) {
  // handling Internet Explorer stupidity with window.event
  // @see http://stackoverflow.com/a/3985882/517705
  var keyDownEvent = event || window.event,
      keycode = (keyDownEvent.which) ? keyDownEvent.which : keyDownEvent.keyCode;
  console.log("key code: " + keycode);
  switch(keycode) {
    case 8: numpad.delete(); break;
    case 13: numpad.select(); break;
    case 27: numpad.hide(); break;
    default:
      if((keycode>=0x60) && (keycode<=0x69)) {
        keycode-=0x30;
      }
      if((keycode>=0x30) && (keycode<=0x39)) {
        numpad.status.value = "";
        var current=numpad.display.value;
        keycode-=0x30;
        if ( current.length < 3) {
          if (current=="0") {
            numpad.display.value = keycode;
          } else {
            numpad.display.value += keycode;
          }      
      }
    }
  }
}

function formToJSON()
{
  var form = document.getElementById("settingsForm");
  var formData = new FormData(form);
  var object = {};
  for(var pair of formData.entries()) {
    console.log("key: "+pair[0]+" value: "+pair[1]); 
  }
  object["wifi_opmode"] = parseInt(formData.get("wifi_opmode"));
  object["wifi_ssid"] = formData.get("wifi_ssid");
  object["wifi_secret"] = formData.get("wifi_secret");
  object["wifi_hostname"] = formData.get("wifi_hostname");
  object["wifi_powersave"] = formData.get("wifi_powersave");
  object["alert_type"] = formData.get("alert_type");
  object["restaurant_id"] = formData.get("restaurant_id");
  object["system_id"] = formData.get("system_id");
  object["tx_frequency"] = formData.get("tx_frequency");
  object["tx_deviation"] = formData.get("tx_deviation");
  object["tx_power"] = formData.get("tx_power");
  
  return JSON.stringify(object);
}

function JSONToForm(form, json)
{
  settings = json;
  console.log(JSON.stringify(json));
  statusSpan.innerHTML="Version: "+json.version;
  switch(json.wifi_opmode) {
    case false: document.getElementById("ap").checked=true; break;
    case true: document.getElementById("sta").checked=true; break;
  }
  document.getElementsByName("wifi_ssid")[0].value=json.wifi_ssid;
  document.getElementsByName("wifi_secret")[0].value=json.wifi_secret;
  document.getElementsByName("wifi_hostname")[0].value=json.wifi_hostname;
  document.getElementsByName("wifi_powersave")[0].value=json.wifi_powersave;
  switch(json.wifi_powersave) {
    case false: document.getElementById("wifi_powersave_off").checked=true; break;
    case true: document.getElementById("wifi_powersave_on").checked=true; break;
  }

  document.getElementsByName("alert_type")[0].value=json.alert_type;
  document.getElementsByName("restaurant_id")[0].value=json.restaurant_id;
  document.getElementsByName("system_id")[0].value=json.system_id;

  document.getElementsByName("tx_frequency")[0].value=json.tx_frequency;
  document.getElementsByName("tx_deviation")[0].value=json.tx_deviation;
  document.getElementsByName("tx_power")[0].value=json.tx_power;
}

function getSettings() {
  var xmlhttp = new XMLHttpRequest();
  var url = "settings.json";

  xmlhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
        settings = JSON.parse(this.responseText);
        JSONToForm("settingsForm", settings);
    }
  };
  xmlhttp.open("GET", url, true);
  xmlhttp.send();
}

function postSettings(json) {
  var xmlhttp = new XMLHttpRequest();
  var url = "settings.json";

  xmlhttp.open("POST", url, true);
  xmlhttp.setRequestHeader("Content-Type", "application/json");
  xmlhttp.onreadystatechange = function () {
      if (xmlhttp.readyState === 4 && xmlhttp.status === 200) {
        settings = JSON.parse(this.responseText); 
      }
      reboot();
    };
  
  xmlhttp.send(json);
}

function page(pager_num) {
  var xmlhttp = new XMLHttpRequest();
  var url = "page?pager_number="+pager_num;

  xmlhttp.onreadystatechange = function() {
    console.log("page: readyState:"+this.readyState+" status:"+this.status);
    if (this.readyState == 4) {
      if (this.status == 200) {
        numpad.status.value="OK";
      } else {
        numpad.status.value="FAIL";
        page(pager_num);
      }
    }
  };
  xmlhttp.open("GET", url, true);
  xmlhttp.send();
}

function reboot() {
  var xmlhttp = new XMLHttpRequest();
  var url = "reboot";

  xmlhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  xmlhttp.open("GET", url, true);
  xmlhttp.send();
}

function factoryreset() {
  var xmlhttp = new XMLHttpRequest();
  var url = "factoryreset";

  xmlhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  xmlhttp.open("GET", url, true);
  xmlhttp.send();
}

settingsButton.onclick  = function()    {
  document.onkeydown = "";
  getSettings();
  modal.style.display = "block";
};
closeModal.onclick = function()         {
  document.onkeydown = checkKeycode;
  modal.style.display = "none"; 
};
saveButton.onclick = function()         {
  document.onkeydown = checkKeycode;
  modal.style.display = "none"; 
  jsonStr=formToJSON();
  console.log(jsonStr);
  postSettings(jsonStr);
};
rebootButton.onclick = function()       {
  modal.style.display = "none"; 
  reboot();
};
factoryresetButton.onclick = function () {
  modal.style.display = "none"; 
  factoryreset();
}
window.onclick = function(event) {
  if (event.target == modal) {
      document.onkeydown = checkKeycode;
      modal.style.display = "none";
  }
}

var numpad = {
  /* [INIT - DRAW THE ON-SCREEN NUMPAD] */
  selector : null, // will hold the entire on-screen numpad
  display : null, // will hold the numpad display
  zero : null, // will hold the zero button
  dot : null, // will hold the dot button
  init : function () {
    // CREATE THE NUMPAD
    numpad.selector = document.createElement("div");
    numpad.selector.id = "numpad-back";
    var wrap = document.createElement("div");
    wrap.id = "numpad-wrap";
    numpad.selector.appendChild(wrap);

    // ATTACH THE NUMBER DISPLAY
    numpad.display = document.createElement("input");
    numpad.display.id = "numpad-display";
    numpad.display.type = "text";
    numpad.display.readOnly = true;
    wrap.appendChild(numpad.display);

    // ATTACH Status Field
    numpad.status = document.createElement("input");
    numpad.status.id = "numpad-status";
    numpad.status.type = "text";
    numpad.status.readOnly = true;
    wrap.appendChild(numpad.status);


    // ATTACH BUTTONS
    var buttons = document.createElement("div"),
        button = null,
        append = function (txt, fn, css) {
          button = document.createElement("div");
          button.innerHTML = txt;
          button.classList.add("numpad-btn");
          if (css) {
            button.classList.add(css);
          }
          button.addEventListener("click", fn);
          buttons.appendChild(button);
        };
    buttons.id = "numpad-btns";
    // First row - 1 to 3, delete.
    for (var i=1; i<=3; i++) {
      append(i, numpad.digit);
    }
    append("&#10502;", numpad.delete, "ng");
    // Second row - 4 to 6, clear.
    for (var i=4; i<=6; i++) {
      append(i, numpad.digit);
    }
    append("C", numpad.reset, "ng");
    // Third row - 7 to 9, cancel.
    for (var i=7; i<=9; i++) {
      append(i, numpad.digit);
    }
    append("&#10006;", numpad.hide, "cx");
    // Last row - 0, dot, ok
    append(0, numpad.digit, "zero");
    numpad.zero = button;
    append(".", numpad.dot);
    numpad.dot = button;
    append("&#10004;", numpad.select, "ok");
    // Add all buttons to wrapper
    wrap.appendChild(buttons);
    document.body.appendChild(numpad.selector);
  },

  /* [ATTACH TO INPUT] */
  attach : function (opt) {
  // attach() : attach numpad to target input field

    var target = document.getElementById(opt.id);
    if (target!=null) {
      // APPEND DEFAULT OPTIONS
      if (opt.readonly==undefined || typeof opt.readonly!="boolean") { opt.readonly = true; }
      if (opt.decimal==undefined || typeof opt.decimal!="boolean") { opt.decimal = true; }
      if (opt.max==undefined || typeof opt.max!="number") { opt.max = 16; }

      // SET READONLY ATTRIBUTE ON TARGET FIELD
      if (opt.readonly) { target.readOnly = true; }

      // ALLOW DECIMALS?
      target.dataset.decimal = opt.decimal ? 1 : 0;

      // MAXIMUM ALLOWED CHARACTERS
      target.dataset.max = opt.max;

      // SHOW NUMPAD ON CLICK
      target.addEventListener("click", numpad.show);
      numpad.selector.classList.add("show");
      var evt={target: target};
      numpad.show(evt);

    } else {
      console.log(opt.id + " NOT FOUND!");
    }
  },

  target : null, // contains the current selected field
  dec : false, // allow decimals?
  max : 3, // max allowed characters
  show : function (evt) {
  // show() : show the number pad

    // Set current target field
    numpad.target = evt.target;

    // Show or hide the decimal button
    numpad.dec = numpad.target.dataset.decimal==1;
    if (numpad.dec) {
      numpad.zero.classList.remove("zeroN");
      numpad.dot.classList.remove("ninja");
    } else {
      numpad.zero.classList.add("zeroN");
      numpad.dot.classList.add("ninja");
    }

    // Max allowed characters
    numpad.max = parseInt(numpad.target.dataset.max);

    // Set display value
    var dv = evt.target.value;
    if (!isNaN(parseFloat(dv)) && isFinite(dv)) {
      numpad.display.value = dv;
    } else {
      numpad.display.value = "";
    }

    // Show numpad
    numpad.selector.classList.add("show");
  },

  hide : function () {
  // hide() : hide the number pad
    numpad.status.value="";
    numpad.selector.classList.remove("show");
  },

  /* [BUTTON ONCLICK ACTIONS] */
  delete : function () {
  // delete() : delete last digit on the number pad
    numpad.status.value="";
    var length = numpad.display.value.length;
    if (length > 0) {
      numpad.display.value = numpad.display.value.substring(0, length-1);
    }
  },

  reset : function () {
  // reset() : reset the number pad
    numpad.status.value="";
    numpad.display.value = "";
  },

  digit : function (evt) {
  // digit() : append a digit
    numpad.status.value="";
    var current = numpad.display.value,
        append = evt.target.innerHTML;

    if (current.length < numpad.max) {
      if (current=="0") {
        numpad.display.value = append;
      } else {
        numpad.display.value += append;
      }
    }
  },

  dot : function () {
  // dot() : add the decimal point (only if not already appended)
    numpad.status.value="";
    if (numpad.display.value.indexOf(".") == -1) {
      if (numpad.display.value=="") {
        numpad.display.value = "0.";
      } else {
        numpad.display.value += ".";
      }
    }
  },

  select : function () {
  // select() : select the current number

    var value = numpad.display.value;

    // No decimals allowed - strip decimal
    if (!numpad.dec && value%1!=0) {
      value = parseInt(value);
    }

    // Put selected value to target field + close numpad
    //numpad.target.value = value;
    page(value);
    numpad.display.value = "";
  }
};

/* [INIT] */
window.addEventListener("load", numpad.init);

getSettings();
window.addEventListener("load", function(){
  numpad.attach({
    id : "container1",
    readonly : false, 
    decimal : false,
    max : 3
  });
});

document.onkeydown = checkKeycode;

