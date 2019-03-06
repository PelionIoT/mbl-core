/*
 * Copyright (c) 2019 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef ResourceDefinitionJson_h_
#define ResourceDefinitionJson_h_

// JSON test strings for testing JSON parser and other tests.
// Use https://codebeautify.org/online-json-editor to generate new strings.

/*
  "1": {
    "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      },
      "112": {
        "mode": "dynamic",
        "type": "string",
        "operations": [
          "get",
          "put",
          "delete"
        ],
        "observable": true,
        "multiple_instance": true
      }
    }
  },
  "2": {
    "21": {
      "211": {
        "mode": "static",
        "type": "integer",
        "value": 999,
        "operations": [
          "get"
        ],
        "multiple_instance": true
      }
    },
    "22": {
      "221": {
        "mode": "dynamic",
        "type": "integer",
        "operations": [
          "get",
          "post",
          "put"
        ],
        "multiple_instance": true,
        "observable": true
      }
    }
  }
}
*/
#define VALID_APP_RESOURCE_DEFINITION_OBJECT_WITH_SEVERAL_OBJECT_INSTANCES_AND_RESOURCES \
  R"({"1" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false}, "112" : { "mode" : "dynamic", "type" : "string", "operations" : ["get","put", "delete"], "observable" : true, "multiple_instance" : true } } }, "2" : { "21" : { "211" : { "mode" : "static", "type" : "integer", "value" : 999 , "operations" : ["get"], "multiple_instance" : true} }, "22" : { "221" : { "mode" : "dynamic", "type" : "integer", "operations" : ["get","post","put"], "multiple_instance" : true, "observable" : true } } } })"

/*
{
  "1": {
    "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      }
    }
  },
  "2": {
    "21": {
      "211": {
        "mode": "static",
        "type": "integer",
        "value": 123456,
        "operations": [
          "get"
        ],
        "multiple_instance": true
      }
    }
  }
}
*/
#define VALID_APP_RESOURCE_DEFINITION_TWO_OBJECTS_WITH_ONE_OBJECT_INSTANCE_AND_ONE_RESOURCE \
  R"({"1" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } } , "2" : { "21" : { "211" : { "mode" : "static", "type" : "integer", "value" : 123456 , "operations" : ["get"], "multiple_instance" : true} } } })"

/*
{
  "8888": {
    "11": {
      "111": {
        "mode": "dynamic",
        "type": "string",
        "operations": [
          "get",
          "put",
          "delete"
        ],
        "observable": true,
        "multiple_instance": false
      },
      "112": {
        "mode": "dynamic",
        "type": "integer",
        "operations": [
          "get",
          "put",
          "delete"
        ],
        "observable": true,
        "multiple_instance": false
      }
    }
  }
}
*/
#define VALID_APP_RESOURCE_DEFINITION_ONE_DYNAMIC_OBJECT_WITH_ONE_OBJECT_INSTANCE_AND_TWO_RESOURCE \
  R"({"8888" : { "11" : { "111" : { "mode" : "dynamic", "type" : "string", "operations" : ["get","put", "delete"], "observable" : true, "multiple_instance" : false },  "112" : { "mode" : "dynamic", "type" : "integer", "operations" : ["get","put", "delete"], "observable" : true, "multiple_instance" : false } } } })"


/*
{
  "Obj1": {},
  "Obj2": {
    "1": {},
    "2": {},
    "3": {}
  },
  "Obj3": {
    "1": {
      "0": {
        "mode": "static",
        "type": "integer",
        "operations": [
          "get"
        ],
        "value": 7
      },
      "1": {
        "mode": "dynamic",
        "type": "string",
        "operations": [
          "get",
          "put"
        ],
        "observable": true
      }
    },
    "2": {
      "101": {
        "mode": "dynamic",
        "type": "integer",
        "operations": [
          "get",
          "post"
        ],
        "multiple_instance": true
      }
    }
  }
}
*/
#define INVALID_APP_RESOURCE_DEFINITION_NOT_3_LEVEL_1 \
  R"({"Obj1" : {}, "Obj2" : { "1" : {}, "2" : {}, "3" : {} }, "Obj3" : { "1" : { "0" : { "mode" : "static", "type" : "integer", "operations" : ["get"], "value" : 7 }, "1" : { "mode" : "dynamic", "type" : "string", "operations" : ["get","put"], "observable" : true } }, "2" : { "101" : { "mode" : "dynamic", "type" : "integer", "operations" : ["get","post"], "multiple_instance" : true } } } })"

/*
{
  "32811": {}
}
*/
#define INVALID_APP_RESOURCE_DEFINITION_NOT_3_LEVEL_2 \
  R"({"32811" : {}})"

/*
{
  "1": {
    "11": {
      "111": {
        "mode": "dynamic",
        "type": "string",
        "operations": [
          "get",
          "put",
          "delete"
        ],
        "multiple_instance": true
      }
    }
  }
}
*/
#define INVALID_APP_RESOURCE_DEFINITION_MISSING_OBSERVABLE_ENTRY \
  R"({"1" : { "11" : { "111" : { "mode" : "dynamic", "type" : "string", "operations" : ["get","put", "delete"], "multiple_instance" : true } } } })"

/*
{
  "1": {
    "11": {
      "111": {
        "mode": "invalid_mode",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      }
    }
  }
}*/
#define INVALID_APP_RESOURCE_DEFINITION_ILLEGAL_RESOURCE_MODE \
  R"({"1" : { "11" : { "111" : { "mode" : "invalid_mode", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } } })"   

/*
{
  "1": {
    "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "invalid_operation"
        ],
        "multiple_instance": false
      }
    }
  }
}*/
#define INVALID_APP_RESOURCE_DEFINITION_ILLEGAL_RESOURCE_OPERATION \
  R"({"1" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["invalid_operation"], "multiple_instance" : false} } } })"; 

/*
{
  "1": {
    "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "bla_bla_type",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      }
    }
  }
}*/
#define INVALID_APP_RESOURCE_DEFINITION_UNSUPPORTED_RESOURCE_TYPE \
  R"({"1" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "bla_bla_type", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } } })"

/*
{
  "1": {
    "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      },
     "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      }
    }
  }
}
*/
#define INVALID_APP_RESOURCE_DEFINITION_TWO_SAME_RESOURCE_NAMES \
  R"({"1" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false}, "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } } })"   

/*
{
  "1": {
    "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      }
    },
   "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      }
    }
  }
}*/
#define INVALID_APP_RESOURCE_DEFINITION_TWO_SAME_OBJECT_INSTANCES \
  R"({"1" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } , "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } } })"   

/*
{
  "1": {
    "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      }
    }
  },
 "1": {
    "11": {
      "111": {
        "mode": "static",
        "resource_type": "reset_button",
        "type": "string",
        "value": "string_val",
        "operations": [
          "get"
        ],
        "multiple_instance": false
      }
    }
  }  
}*/
#define INVALID_APP_RESOURCE_DEFINITION_TWO_SAME_OBJECT \
  R"({"1" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } }, "1" : { "11" : { "111" : { "mode" : "static", "resource_type" : "reset_button", "type" : "string", "value": "string_val", "operations" : ["get"], "multiple_instance" : false} } } })"



#endif // ResourceDefinitionJson_h_
