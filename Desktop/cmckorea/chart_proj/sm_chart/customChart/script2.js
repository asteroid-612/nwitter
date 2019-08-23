var app = angular.module('myApp', ['zingchart-angularjs']);
//var app2 = angular.module('myApp2', []);
var mapDataListLat = [];
var mapDataListLng = [];
//데이터 넘길 값
var paramData = {
  "blogid": "test1",
  "restApiKey": "7b2f8c64c5395060b99bbf650b674e33",
}
paramData = angular.fromJson(paramData);

//controller2
app.controller('testController', function($scope, $http) {
  var temp = location.href.split("?");
  var data = temp[1].split("&");
  var select = data[0];
  console.log(select)
  var title = decodeURIComponent(data[1]);
  console.log(select, title);

  document.getElementById("title").innerHTML = title;

  $scope.mapDataId = [];
  $scope.mapDataListLat = [];
  $scope.mapDataListLng = [];
  $scope.mapDataName = [];
  $scope.mapDataCategory = [];
  $scope.mapDataImagae = [];
  $scope.uniqCategory = [];
  $scope.countCategory = [];

  $scope.groupIdNumber1 = [];
	$scope.groupIdNumber2 = [];
	$scope.groupIdNumberElse = [];

  //파라미터 값 선언
  $scope.paramData = {
    "blogid": "test1",
    "restApiKey": "7b2f8c64c5395060b99bbf650b674e33",
  }
  $scope.paramData = angular.fromJson($scope.paramData);

  $scope.postRequest = function() {

    $scope.push = [];
    $scope.group = [];
    $http.get("http://www.mapplerk3.com/rest/v1/blogs/test1/dftBlogSettingInfo?restApiKey=eeeeb01c378357d1ad1313a5983ffc4e").
    then(function(response) {
      angular.forEach(response, function(list1) {
        angular.forEach(list1.mapDftSettingInfo, function(list2) {
          $scope.push.push(list2);
          angular.forEach($scope.push[14], function(list3) {
            $scope.groupVal = {};
            $scope.groupVal["groupN"] = list3.g_c_seq;
            $scope.groupVal["val"] = list3.category_name;
            $scope.group.push($scope.groupVal);
            //카테고리 이름 같은 것은 같은 배열로 할당
          })
        })
      })
      console.log("Successfully get data");
      //console.log(response);
      $scope.group1 = [];
      $scope.group2 = [];
      for (var i = 0; i < ($scope.group.length); i++) {
        //전의 것과 비교해서 groupN가 다르면 group2에 val 넣어주기
        if ($scope.group[i].groupN != $scope.group[0].groupN) {
          $scope.group2.push($scope.group[i].val);
          continue;
        }
        $scope.group1.push($scope.group[i].val);
      }

    $http({
      method: "POST",
      url: "http://mapplerk2.com/rest/v1/maps/test1",
      data: $scope.paramData,
      dataType: "json",
      headers: {
        'Content-Type': "application/x-www-form-urlencoded"
      }
    }).then(function successCallback(response) {
      console.log("Successfully POST-ed data");
      var i = 0;
      angular.forEach(response, function(list) {
        angular.forEach(list.mapDataList, function(list) {
            $scope.mapDataId.push(parseInt(list.oid));
            $scope.mapDataListLat.push(parseFloat(list.lat));
            $scope.mapDataListLng.push(parseFloat(list.lng));
            $scope.mapDataName.push(list.name);
            $scope.mapDataCategory.push(list.category);
            $scope.mapDataImagae.push(list.s3ThumbObjUrl);
            if($scope.group1.includes(list.category)){
              $scope.groupIdNumber1.push(i);
            }
            else if($scope.group2.includes(list.category)) {
              $scope.groupIdNumber2.push(i);
            }else {
              $scope.groupIdNumberElse.push(i);
            }
            i++;
        })
      })
      console.log($scope.groupIdNumber1);
      console.log($scope.groupIdNumber2);
      $.each($scope.mapDataCategory, function(i, el) {
        if ($.inArray(el, $scope.uniqCategory) === -1) $scope.uniqCategory.push(el);
      });
      $scope.uniqCategory = $scope.uniqCategory.sort();

      function getCount(arr, val) {
        var ob = {};
        var len = arr.length;
        for (var k = 0; k < len; k++) {
          if (ob.hasOwnProperty(arr[k])) {
            ob[arr[k]]++;
            continue;
          }
          ob[arr[k]] = 1;
        }
        return ob[val];
      }
      //run test
      for (var i = 0; i < $scope.uniqCategory.length; i++) {
        $scope.countCategory[i] = getCount($scope.mapDataCategory, $scope.uniqCategory[i]);
        console.log($scope.uniqCategory[i]);
        console.log($scope.countCategory[i]);
      }

      //console.log($scope.uniqCategory);
      $scope.myJson = {
        gui: {
          contextMenu: {
            button: {
              visible: 0
            }
          }
        },
        backgroundColor: "#434343",
        globals: {
          shadow: false,
          fontFamily: "Helvetica"
        },
        type: "area",

        legend: {
          layout: "x4",
          backgroundColor: "transparent",
          borderColor: "transparent",
          marker: {
            borderRadius: "50px",
            borderColor: "transparent"
          },
          item: {
            fontColor: "white"
          }

        },
        scaleX: {
          maxItems: 8,
          transform: {
            type: 'text'
          },
          zooming: true,
          values: $scope.uniqCategory,
          lineColor: "white",
          lineWidth: "1px",
          tick: {
            lineColor: "white",
            lineWidth: "1px"
          },
          item: {
            fontColor: "white"
          },
          guide: {
            visible: false
          }
        },
        scaleY: {
          lineColor: "white",
          lineWidth: "1px",
          tick: {
            lineColor: "white",
            lineWidth: "1px"
          },
          guide: {
            lineStyle: "solid",
            lineColor: "#626262"
          },
          item: {
            fontColor: "white"
          },
        },
        tooltip: {
          visible: false
        },
        crosshairX: {
          scaleLabel: {
            backgroundColor: "#fff",
            fontColor: "black"
          },
          plotLabel: {
            backgroundColor: "#434343",
            fontColor: "#FFF",
            _text: "Number of hits : %v"
          }
        },
        plot: {
          lineWidth: "2px",
          aspect: "spline",
          marker: {
            visible: false
          }
        },
        series: [{
            text: "All Sites",
            values: $scope.mapDataId,       // all데이터...
            backgroundColor1: "#77d9f8",
            backgroundColor2: "#272822",
            lineColor: "#40beeb"
          }, {
            text: "Site 1",
            values: $scope.mapDataListLat,  // 그룹데이터1 lat값 들어가게
            backgroundColor1: "#4AD8CC",
            backgroundColor2: "#272822",
            lineColor: "#4AD8CC"
          }, {
            text: "Site 2",
            values: $scope.mapDataListLng,  // 그룹데이터2
            backgroundColor1: "#1D8CD9",
            backgroundColor2: "#1D8CD9",
            lineColor: "#1D8CD9"
          },
          {
            text: "Site 3",
            values: $scope.countCategory,  // 그룹데이터3
            backgroundColor1: "#D8CD98",
            backgroundColor2: "#272822",
            lineColor: "#D8CD98"
          }
        ]
      }
    }, function errorCallback(response) {
      console.log("POST-ing of data failed");
    });
  });
  };
});
