var app = angular.module('myApp', ['zingchart-angularjs']);
//var app2 = angular.module('myApp2', []);

//controller
app.controller('testController', function($scope, $http) {
  var temp = location.href.split("?");
  var data = temp[1].split("&");
  var select = data[0];
  console.log(select)
  var title = decodeURIComponent(data[1]);
  console.log(select, title);

  // document.getElementById("title").innerHTML = title;

  $scope.mapDataId = [];
  $scope.mapDataListLat = [];
  $scope.mapDataListLng = [];
  $scope.mapDataName = [];
  $scope.mapDataCategory = [];
  $scope.mapDataImage = [];
  $scope.uniqCategory = [];
  $scope.countCategory = [];
  //파라미터 값 선언
  $scope.paramData = {
    "blogid": "test1",
    "restApiKey": "7b2f8c64c5395060b99bbf650b674e33",
  }
  $scope.paramData = angular.fromJson($scope.paramData);

  //post 함수
  $scope.postRequest = function() {
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
      angular.forEach(response, function(list) {
        angular.forEach(list.mapDataList, function(list) {
          $scope.mapDataId.push(parseInt(list.oid));
          $scope.mapDataListLat.push(parseFloat(list.lat));
          $scope.mapDataListLng.push(parseFloat(list.lng));
          $scope.mapDataName.push(list.name);
          $scope.mapDataCategory.push(list.category);
          $scope.mapDataImage.push(list.s3ThumbObjUrl);
        })
      })

      // 카테고리 갯수 구하기 위한 함수
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
      $scope.myJson = {
        title: {
          text: title,
          fontSize: 30,
          fontColor: "#fff"
        },
        backgroundColor: "#2bbb9a",
        globals: {
          shadow: false,
          fontFamily: "Arial"
        },
        type: "line",
        scaleX: {
          maxItems: 8,
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
          },
          values: $scope.uniqCategory
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
            lineColor: "#249178"
          },
          item: {
            fontColor: "white"
          },
        },
        tooltip: {
          visible: false
        },
        crosshairX: {
          lineColor: "#fff",
          scaleLabel: {
            backgroundColor: "#fff",
            fontColor: "#323232"
          },
          plotLabel: {
            backgroundColor: "#fff",
            fontColor: "#323232",
            text: "%v",
            borderColor: "transparent"
          }
        },
        plot: {
          lineWidth: "2px",
          lineColor: "#FFF",
          aspect: "spline",
          marker: {
            visible: false
          }
        },
        series: [{
          //벨류 그대로 넣어주면 끝.
          values: $scope.countCategory
        }]
      }
    }, function errorCallback(response) {
      console.log("POST-ing of data failed");
    });
  };
});
