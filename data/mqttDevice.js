/* updates sensor-, actor-, induction- and mqtt status, called periodically */
function refresh() {
  $.get("/mqttStatus", function (data) {
    $("#mqttStatus").html(data);
  });
  $.get("/getOtherPins", function (data) {
    $("#otherPins").html(data);
  });
  $.get("/reqSensors", function (data) {
    let content = "";
    data.forEach(function (element) {
      let sensor =
        "<li class='list-group-item justify-content-between align-items-center d-flex'> ";
      sensor += "<span class='badge badge-light'> ";
      if (element.value === "ERR") {
        sensor += "Error";
      } else {
        if (element.type === "PTSensor" || element.type === "OneWire") {
          sensor += element.value + "°C";
        } else if (element.type === "Distance") {
          sensor += element.value + " cm";
        } else {
          sensor += element.value + " unknown unit";
        }
      }
      sensor += " </span> <span class='badge badge-light'> ";
      sensor += element.mqtt;
      sensor += " </span> <span class='badge badge-light'> ";
      sensor += element.type;
      sensor +=
        " </span> <a href='' class='badge badge-warning' data-toggle='modal' data-target='#sensor_modal' data-id='";
      if (element.type === "PTSensor" || element.type === "OneWire") {
        sensor += element.id;
      } else if (element.type === "Distance") {
        sensor += 1; // CONVENTION: means: sensor exists
        $("#addDistanceSensor").hide();
      }
      sensor += "' data-type='";
      sensor += element.type;
      sensor += "'> Edit </a> </li>";
      content += sensor;
    });
    $("#listSensors").html(content);
  });
  $.get("/reqActors", function (data) {
    let content = "";
    data.forEach(function (element, index) {
      let actor =
        "<li class='list-group-item d-flex justify-content-between align-items-center'> ";
      actor += " </span> <span class='badge ";
      if (element.status == true) {
        actor += "badge-success'> ON: ";
        actor += element.power;
        actor += "%";
      } else {
        actor += "badge-danger'> OFF";
      }
      actor += "</span> <span class='badge badge-light'>";
      actor += element.mqtt;
      actor += "</span> <span class='badge badge-light'> PIN ";
      actor += element.pin;
      actor +=
        "</span> <a href='' class='badge badge-warning' data-toggle='modal' data-target='#actor_modal' data-id='";
      actor += index;
      actor += "'> Edit </a> </li>";
      content += actor;
    });
    $("#listActors").html(content);
  });
  $.get("/reqInduction", function (data) {
    let inductionRender =
      "<li class='list-group-item d-flex justify-content-between align-items-center'> ";
    if (data.enabled) {
      inductionRender += " Relais Status <span class='badge";
      if (data.relayOn) {
        inductionRender += " badge-success'> ON";
      } else {
        inductionRender += " badge-danger'> OFF";
      }
      inductionRender +=
        "</span> </li> <li class='list-group-item d-flex justify-content-between align-items-center'> Power Requested <span class='badge badge-success'>";
      inductionRender += data.power;
      inductionRender +=
        "%</span> </li> <li class='list-group-item d-flex justify-content-between align-items-center'> Current Power Level <span class='badge badge-success'> P";
      inductionRender += data.powerLevel;
      inductionRender +=
      "</span> </li> <li class='list-group-item d-flex justify-content-between align-items-center'> MQTT Topic <span class='badge badge-light'> ";
    inductionRender += data.topic;
      inductionRender +=
        " </span> </li> <li class='list-group-item d-flex justify-content-between align-items-center'> </li>";
    } else {
      inductionRender += "Induction Cooker Disabled </li>";
    }
    $("#inductionCooker").html(inductionRender);
  });
}

/* click on edit/add actor button */
$("#actor_modal").on("show.bs.modal", function (event) {
  var button = $(event.relatedTarget);
  var actorid = button.data("id");
  var actorscript;
  var actorinverted;
  $.ajax({
    url: `/reqActorConfig?id=${actorid}&req=script`,
    type: "get",
    async: false,
    cache: false,
    success: function (data) {
      actorscript = data;
    },
  });
  $.ajax({
    url: `/reqActorConfig?id=${actorid}&req=inverted`,
    type: "get",
    async: false,
    cache: false,
    success: function (data) {
      // fixMe: Let firmware return boolean values
      if(data == "1"){
        actorinverted = true;
      }
      else{
        actorInverted = false;
      }
    },
  });
  var modal = $(this);
  modal.attr("actor_id", actorid);
  modal.find("#modal_actor_script").val(actorscript);
  $("#modal_actor_pin").load("/reqActorPins?id=" + actorid);
  modal.find("#modal_actor_inverted").prop("checked", actorinverted);

});

/* click on save actor button in edit window */
$("#modal_actor_btn_save").click(function () {
  var modal = $("#actor_modal");
  var actorscript = modal.find("#modal_actor_script").val();
  var actorpin = modal.find("#modal_actor_pin").val();
  var actorInverted = modal.find("#modal_actor_inverted").prop("checked");
  var actorid = modal.attr("actor_id");
  $.ajax({
    url: `/setActor?id=${actorid}&script=${actorscript}&pin=${actorpin}&inverted=${actorInverted}`,
    type: "POST",
    async: false,
    cache: false,
  });
  modal.modal("hide");
});

/* click on delete actor button in edit window */
$("#modal_actor_btn_delete").click(function () {
  var modal = $("#actor_modal");
  var actorid = modal.attr("actor_id");
  $.ajax({
    url: "/delActor?id=" + actorid,
    type: "POST",
    async: false,
    cache: false,
  });
  modal.modal("hide");
});

/* click on add/edit sensor button */
$("#sensor_modal").on("show.bs.modal", function (event) {
  // first get infos from button and transfer to window
  var button = $(event.relatedTarget);
  var sensorId = button.data("id");
  var sensorType = button.data("type");
  var modal = $(this);
  modal.attr("sensor_id", sensorId);
  modal.attr("sensor_type", sensorType);
  // reset fields for new sensor creation request
  if (sensorId == -1) {
    modal.find("#modal_sensor_topic").val("");
    modal.find("#modal_sensor__PT_offset").val("");
    modal.find("#modal_sensor_one_wire_offset").val("");
  }
  if (sensorType == "OneWire") {
    // ui adjustments
    $("#modal_sensor_one_wire_sensor").show();
    $("#modal_sensor_pt_sensor").hide();
    // get available addresses
    $("#modal_sensor_address").load("/reqSearchSensorAdresses?id=" + sensorId);
    // check if sensorId is valid (-1 determines a "create new sensor" call)
    if (sensorId != -1) {
      $.ajax({
        url: "/reqSensorConfig?id=" + sensorId + "&type=" + sensorType,
        type: "get",
        async: false,
        cache: false,
        context: this,
        success: function (data) {
          var modal = $(this);
          modal.find("#modal_sensor_topic").val(data.topic);
          modal.find("#modal_sensor_one_wire_offset").val(data.offset);
        },
      });
    }
  } else if (sensorType == "PTSensor") {
    // ui adjustments
    $("#modal_sensor_one_wire_sensor").hide();
    $("#modal_sensor_pt_sensor").show();
    // load pins
    $("#modal_sensor_cs_pin").load(
      "/reqSensorPins?id=" + sensorId + "&type=" + sensorType
    );
    // check if sensorId is valid (-1 determines a "create new sensor" call)
    if (sensorId != -1) {
      $.ajax({
        url: "/reqSensorConfig?id=" + sensorId + "&type=" + sensorType,
        type: "get",
        dataType: "json",
        async: false,
        cache: false,
        context: this,
        success: function (data) {
          var modal = $(this);
          modal.find("#modal_sensor_topic").val(data.topic);
          modal.find("#modal_sensor__PT_offset").val(data.offset);
          modal.find("#modal_sensor_number_of_wires").val(data.numberOfWires);
        },
      });
    }
  } else if (sensorType == "Distance") {
    // ui adjustments
    $("#modal_sensor_one_wire_sensor").hide();
    $("#modal_sensor_pt_sensor").hide();
    // check if sensorId is valid (-1 determines a "create new sensor" call)
    if (sensorId != -1) {
      $.ajax({
        url: "/reqSensorConfig?id=" + sensorId + "&type=" + sensorType,
        type: "get",
        dataType: "json",
        async: false,
        cache: false,
        context: this,
        success: function (data) {
          var modal = $(this);
          modal.find("#modal_sensor_topic").val(data.topic);
        },
      });
    }
  }
});

/* click on save sensor button in edit window */
$("#modal_sensor_btn_save").click(function () {
  var modal = $("#sensor_modal");
  var sensorTopic = modal.find("#modal_sensor_topic").val();

  var sensorId = modal.attr("sensor_id");
  var sensorType = modal.attr("sensor_type");
  if (sensorType == "OneWire") {
    var sensorAddress = modal.find("#modal_sensor_address").val();
    var sensorOffset = modal.find("#modal_sensor_one_wire_offset").val();
    $.ajax({
      url:
        `/setSensor?type=${sensorType}&topic=${sensorTopic}&id=${sensorId}` +
        `&address=${sensorAddress}&sensorOffset=${sensorOffset}`,
      type: "POST",
      async: false,
      cache: false,
    });
  } else if (sensorType == "PTSensor") {
    var csPin = modal.find("#modal_sensor_cs_pin").val();
    var numberOfWires = modal.find("#modal_sensor_number_of_wires").val();
    var sensorOffset = modal.find("#modal_sensor__PT_offset").val();
    $.ajax({
      url:
        `/setSensor?type=${sensorType}&topic=${sensorTopic}&id=${sensorId}` +
        `&cspin=${csPin}&numberOfWires=${numberOfWires}&sensorOffset=${sensorOffset}`,
      type: "POST",
      async: false,
      cache: false,
    });
  } else if (sensorType == "Distance") {
    $.ajax({
      url: `/setSensor?type=${sensorType}&topic=${sensorTopic}`,
      type: "POST",
      async: false,
      cache: false,
    }).done(function (msg) {
      if (sensorId == -1) {
        modal.modal("hide");
        alert(msg + "\n Page will reloaded now!");
        window.location.reload(true);
      }
    });
  }
  modal.modal("hide");
});

/* click on delete sensor button in edit window */
$("#modal_sensor_btn_delete").click(function () {
  var modal = $("#sensor_modal");
  var sensorId = modal.attr("sensor_id");
  var sensorType = modal.attr("sensor_type");
  $.ajax({
    url: "/delSensor?id=" + sensorId + "&type=" + sensorType,
    type: "POST",
    async: false,
    cache: false,
  });
  if (sensorType === "Distance") {
    $("#addDistanceSensor").show();
  }
  modal.modal("hide");
});

/* click on update OneWire sensor button in edit window */
$("#modal_sensor_address_refresh").click(function () {
  var modal = $("#sensor_modal");
  var sensorId = modal.attr("sensor_id");
  $("#modal_sensor_address").load("/reqSearchSensorAdresses?id=" + sensorId);
});

/* click on edit induction button */
$("#induction_modal").on("show.bs.modal", function () {
  var mqtttopic;
  var isEnabled;
  var delay;

  $.ajax({
    url: "/reqInductionConfig?req=isEnabled",
    type: "get",
    async: false,
    cache: false,
    success: function (data) {
      isEnabled = data;
    },
  });
  $.ajax({
    url: "/reqInductionConfig?req=topic",
    type: "get",
    async: false,
    cache: false,
    success: function (data) {
      mqtttopic = data;
    },
  });
  $.ajax({
    url: "/reqInductionConfig?req=delay",
    type: "get",
    async: false,
    cache: false,
    success: function (data) {
      delay = data;
    },
  });

  $("#modal_induction_pinwhite").load("/reqInductionConfig?req=pins&id=0");
  $("#modal_induction_pinyellow").load("/reqInductionConfig?req=pins&id=1");
  $("#modal_induction_pinblue").load("/reqInductionConfig?req=pins&id=2");

  var modal = $(this);
  modal.find("#modal_induction_script").val(mqtttopic);
  modal.find("#modal_induction_delay").val(delay);
  if (isEnabled == "1") {
    modal.find("#modal_induction_enabled").prop("checked", true);
  } else {
    modal.find("#modal_induction_enabled").prop("checked", false);
  }
  var modal = $(this);
});

/* click on save induction button in edit window */
$("#modal_induction_btn_save").click(function () {
  var modal = $("#induction_modal");

  var mqtttopic = modal.find("#modal_induction_script").val();
  var pin_white = modal.find("#modal_induction_pinwhite").val();
  var pin_blue = modal.find("#modal_induction_pinblue").val();
  var pin_yellow = modal.find("#modal_induction_pinyellow").val();
  var delay = modal.find("#modal_induction_delay").val();

  if (modal.find("#modal_induction_enabled").prop("checked") == true) {
    var isenabled = "1";
  } else {
    var isenabled = "0";
  }

  $.ajax({
    url:
      `/setIndu?enabled=${isenabled}&topic=${mqtttopic}&pinwhite=${pin_white}` +
      `&pinyellow=${pin_yellow}&pinblue=${pin_blue}&delay=${delay}`,
    type: "POST",
    async: false,
    cache: false,
  });
  modal.modal("hide");
});

/* click on "Configure Device" button */
$("#config_modal").on("show.bs.modal", function () {
  var modal = $(this);
  $.ajax({
    url: "/getSysConfig",
    type: "get",
    dataType: "json",
    async: false,
    cache: false,
    success: function (data) {
      modal.find("#modal_mqtt_address").val(data.mqttAddress);
      modal.find("#modal_use_display").prop("checked", data.useDisplay);
      modal.find("#modal_SDA_pin").html(data.SDAPin);
      modal.find("#modal_SCL_pin").html(data.SCLPin);
    },
  });
});

/* click on "Save Changes" in "Configure Device" view */
$("#modal_config_btn_save").click(function () {
  var modal = $("#config_modal");

  var mqttAddress = modal.find("#modal_mqtt_address").val();
  var SDAPin = modal.find("#modal_SDA_pin").val();
  var SCLPin = modal.find("#modal_SCL_pin").val();
  var useDisplay = modal.find("#modal_use_display").prop("checked");

  $.ajax({
    url:
      "/setSysConfig?mqttAddress=" +
      mqttAddress +
      "&useDisplay=" +
      useDisplay +
      "&SDAPin=" +
      SDAPin +
      "&SCLPin=" +
      SCLPin,
    type: "POST",
    async: false,
    cache: false,
  }).done(function (msg) {
    modal.modal("hide");
    alert(msg + "\n Page will reloaded now!");
    window.location.reload(true);
  });
});

/* after initial loading is done, once get the firmware version */
$.get("/version", function (data) {
  $("#version").html("Firmware Device: " + data);
});
