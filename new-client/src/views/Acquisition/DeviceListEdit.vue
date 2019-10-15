<template>
  <div>
    <v-data-iterator
      :items="devices"
      :items-per-page="devicesPerPage"
      :no-data-text="$t('noDataText')"
      :footer-props="{ itemsPerPageOptions: [] }"
    >
      <template v-slot:default="props">
        <v-row>
          <v-col
            v-for="(device, indexDevice) in props.items"
            :key="indexDevice"
            cols="12"
            md="6"
            lg="4"
          >
            <!-- For Device List -->
            <v-card class="pa-1">
              <v-card-text>
                <v-container class="ma-0 pa-0">
                  <v-row no-gutters>
                    <v-col cols="11">{{getIndex(indexDevice, props) + 1}}</v-col>
                    <v-col cols="1">
                      <v-btn
                        icon
                        @click="deleteDevice(getIndex(indexDevice, props))"
                        :disabled="!editable"
                      >
                        <v-icon dark>mdi-close</v-icon>
                      </v-btn>
                    </v-col>
                  </v-row>
                  <v-row no-gutters>
                    <!-- Device Type -->
                    <v-col cols="6" class="pa-1">
                      <v-select
                        v-model="device.type"
                        :disabled="!editable"
                        :items="deviceDefine.deviceTypes"
                        :label="$t('Devices.Type.label')"
                      />
                    </v-col>
                    <!-- Device Name -->
                    <v-col cols="6" class="pa-1">
                      <v-text-field
                        v-model="device.name"
                        :disabled="!editable"
                        :rules="[rules.required, rules.onlyIdentifier]"
                        :counter="deviceDefine.maxDeviceNameLength"
                        :hint="$t('Devices.Name.hint')"
                        :label="$t('Devices.Name.label')"
                      />
                    </v-col>
                  </v-row>
                </v-container>

                <!-- Device Parameters -->
                {{$t('Devices.parameters')}}
                <v-container class="ma-0 pa-0">
                  <v-row
                    v-for="(param, indexParam) in device.parameters"
                    :key="indexDevice+'-'+indexParam"
                    no-gutters
                  >
                    <!-- Delete a Parameter -->
                    <v-col cols="2">
                      <v-btn
                        icon
                        class="mt-3"
                        @click="deleteParameter(getIndex(indexDevice, props), indexParam)"
                        :disabled="!editable"
                      >
                        <v-icon dark>mdi-minus</v-icon>
                      </v-btn>
                    </v-col>
                    <v-col cols="10" class="pa-1">
                      <v-text-field
                        v-model="param.value"
                        :disabled="!editable"
                        :rules="deviceDefine.parameterRules[param.type]"
                        :label="param.type"
                      ></v-text-field>
                    </v-col>
                  </v-row>
                  <!-- Add a Parameter -->
                  <v-row>
                    <v-col cols="2">
                      <v-menu transition="slide-y-transition" top offset-y close-on-content-click>
                        <template v-slot:activator="{ on }">
                          <v-btn icon class="mt-2" v-on="on" :disabled="!editable">
                            <v-icon dark>mdi-plus</v-icon>
                          </v-btn>
                        </template>
                        <v-list>
                          <v-list-item
                            v-for="(paramType, indexParamType) in deviceDefine.parameterTypes[device.type]"
                            :key="indexParamType"
                            @click.prevent="addParameter(getIndex(indexDevice, props), paramType)"
                          >
                            <v-list-item-title>{{ paramType }}</v-list-item-title>
                          </v-list-item>
                        </v-list>
                      </v-menu>
                    </v-col>
                    <v-col cols="10"></v-col>
                  </v-row>
                </v-container>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </template>
    </v-data-iterator>

    <!-- Add a Device -->
    <v-row>
      <v-spacer />
      <v-menu transition="slide-y-transition" top offset-y close-on-content-click>
        <template v-slot:activator="{ on }">
          <v-btn icon class="mt-2" v-on="on" :disabled="!editable">
            <v-icon dark>mdi-plus</v-icon>
          </v-btn>
        </template>
        <v-list>
          <v-list-item
            v-for="(deviceType, indexDeviceType) in deviceDefine.deviceTypes"
            :key="indexDeviceType"
            @click.prevent="addDevice(deviceType)"
          >
            <v-list-item-title>{{ deviceType }}</v-list-item-title>
          </v-list-item>
        </v-list>
      </v-menu>
    </v-row>
  </div>
</template>

<script lang="ts">
import { Component, Vue, Prop } from 'vue-property-decorator';
import { IParameter, IDevice, IDeviceInfo } from '@/core/tronApi';
import deviceDefine from '@/core/deviceDefine';
import inputRules from '@/core/inputRules';

@Component
export default class DeviceListEdit extends Vue {
  @Prop()
  devices!: Array<IDevice>;

  @Prop()
  editable!: boolean;

  rules = inputRules;

  deviceDefine = deviceDefine;

  getIndex(indexInPage: number, props: any): number {
    return indexInPage + props.pagination.pageStart;
  }

  get devicesPerPage(): number {
    if (this.$vuetify.breakpoint.smAndDown) {
      return 1;
    }
    if (this.$vuetify.breakpoint.md) {
      return 2;
    }
    return 3;
  }

  deleteParameter(indexDevice: number, indexParam: number) {
    this.devices[indexDevice].parameters.splice(indexParam, 1);
  }

  addParameter(indexDevice: number, ParamType: string) {
    const device = this.devices[indexDevice];
    const newParam: IParameter = { type: ParamType, value: '' };
    device.parameters.push(newParam);
  }

  deleteDevice(indexDevice: number) {
    this.devices.splice(indexDevice, 1);
  }

  addDevice(deviceType: string) {
    this.devices.push({
      type: deviceType,
      name: '',
      parameters: [],
    });
  }
}
</script>
