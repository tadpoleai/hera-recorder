<template>
  <v-data-iterator
    :items="devices"
    :items-per-page="devicesPerPage"
    :no-data-text="$t('noDataText')"
    :footer-props="{ itemsPerPageOptions: [] }"
  >
    <template v-slot:default="props">
      <v-row>
        <v-col v-for="(device, index) in props.items" :key="index" cols="12" lg="6">
          <!-- For Device List -->
          <v-card class="pa-2">
            <!-- Device Type -->
            <v-card-text>
              <v-select
                v-model="device.type"
                :disabled="deviceChangeDisabled"
                :items="deviceDefine.deviceTypes"
                :label="$t('Devices.Type.label')"
                outlined
              ></v-select>
              <!-- Device Name -->
              <v-text-field
                v-model="device.name"
                :disabled="deviceChangeDisabled"
                :rules="[rules.required, rules.onlyIdentifier]"
                :counter="maxStoragePathLength"
                :hint="$t('Devices.Name.hint')"
                :label="$t('Devices.Name.label')"
                outlined
              ></v-text-field>
            </v-card-text>
            <v-divider />

            <!-- Device Parameters -->
            <v-card-text>
              <v-container class="ma-0 pa-0">
                <v-row v-for="(param, index2) in device.parameters" :key="index+'-'+index2">
                  <v-col cols="5">
                    <v-select
                      v-model="param.name"
                      :disabled="deviceChangeDisabled"
                      :items="deviceDefine.parameterTypes[device.type]"
                      :label="$t('Devices.Parameter.Name.label')"
                    ></v-select>
                  </v-col>
                  <v-col cols="5">
                    <v-text-field
                      v-model="param.value"
                      :disabled="deviceChangeDisabled"
                      :rules="deviceDefine.parameterRules[param.name]"
                      :label="$t('Devices.Parameter.Value.label')"
                    ></v-text-field>
                  </v-col>
                  <!-- Delete a Parameter -->
                  <v-col cols="2">
                    <v-btn icon class="mt-2" @click="deleteParameter(device, index2)">
                      <v-icon dark>mdi-minus</v-icon>
                    </v-btn>
                  </v-col>
                </v-row>
                <!-- Add a Parameter -->
                <v-row>
                  <v-col cols="10"></v-col>
                  <v-col cols="2">
                    <v-btn icon class="mt-2">
                      <v-icon dark>mdi-plus</v-icon>
                    </v-btn>
                  </v-col>
                </v-row>
              </v-container>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>
    </template>
  </v-data-iterator>
</template>

<script lang="ts">
import { Component, Vue, Prop } from 'vue-property-decorator';
import { IParameter, IDevice } from '@/core/tronApi';
import deviceDefine from '@/core/deviceDefine';
import inputRules from '@/core/inputRules';

@Component
export default class DeviceListEdit extends Vue {
  @Prop()
  devices: Array<IDevice>;

  @Prop()
  deviceChangeDisabled!: boolean;

  rules = inputRules;

  deviceDefine = deviceDefine;

  maxDeviceNameLength = '32';

  get devicesPerPage(): number {
    if (this.$vuetify.breakpoint.mdAndDown) {
      return 1;
    }
    return 2;
  }

  deleteParameter(device: IDevice, index: number) {
    device.parameters.splice(index, 1);
  }
}
</script>
